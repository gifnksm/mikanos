/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include "asmfunc.h"
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

char pixel_writer_buf[sizeof(RgbResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

__attribute__((format(printf, 1, 2))) int printk(const char *format, ...) {
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  StartLapicTimer();
  console->PutString(s);
  auto elapsed = LapicTimerElapsed();
  StopLapicTimer();

  sprintf(s, "[%9d]", elapsed);
  console->PutString(s);
  return result;
}

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

unsigned int mouse_layer_id;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
  layer_manager->MoveRelative(mouse_layer_id, {displacement_x, displacement_y});
  StartLapicTimer();
  layer_manager->Draw();
  auto elapsed = LapicTimerElapsed();
  StopLapicTimer();
  printk("MouseObserver: elapsed = %u\n", elapsed);
}

void SwitchEhci2Xhci(const pci::Device &xhc_dev) {
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
        0x8086 == pci::ReadVendorId(pci::devices[i])) {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);          // USB3_PSSEN
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);  // XUSB2PRM
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);           // XUSB2PR
  Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n", superspeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller *xhc;

struct Message {
  enum Type {
    kInterruptXhci,
  } type;
};

ArrayQueue<Message> *main_queue;

__attribute__((interrupt)) void IntHandlerXhci(InterruptFrame *frame) {
  main_queue->Push(Message{Message::kInterruptXhci});
  NotifyEndOfInterrupt();
}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &frame_buffer_config_ref,
                                   const MemoryMap &memory_map_ref) {
  FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
  MemoryMap memory_map{memory_map_ref};

  switch (frame_buffer_config.pixel_format) {
  case kPixelRgbResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf) RgbResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  case kPixelBgrResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf) BgrResv8BitPerColorPixelWriter{frame_buffer_config};
    break;
  }

  DrawDesktop(*pixel_writer);

  console = new (console_buf) Console{kDesktopFgColor, kDesktopBgColor};
  console->SetWriter(pixel_writer);
  printk("Welcome to MikanOS!\n");
  SetLogLevel(kWarn);

  InitializeLapicTimer();

  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;
  SetDsAll(0);
  SetCsSs(kernel_cs, kernel_ss);

  SetupIdentityPageTable();

  ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;

  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
  uintptr_t available_end = 0;
  for (uintptr_t iter = memory_map_base; iter < memory_map_base + memory_map.map_size;
       iter += memory_map.descriptor_size) {
    auto desc = reinterpret_cast<const MemoryDescriptor *>(iter);
    if (available_end < desc->physical_start) {
      memory_manager->MarkAllocated(FrameId{available_end / kBytesPerFrame},
                                    (desc->physical_start - available_end) / kBytesPerFrame);
    }

    const auto physical_end = desc->physical_start + desc->number_of_pages * kUefiPageSize;
    if (IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
    } else {
      memory_manager->MarkAllocated(FrameId{desc->physical_start / kBytesPerFrame},
                                    desc->number_of_pages * kUefiPageSize / kBytesPerFrame);
    }
  }
  memory_manager->SetMemoryRange(FrameId{1}, FrameId{available_end / kBytesPerFrame});

  if (auto err = InitializeHeap(*memory_manager)) {
    Log(kError, "failed to allocate pages: %s at %s:%d\n", err.Name(), err.File(), err.Line());
    exit(1);
  }

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;

  auto err = pci::ScanAllBus();
  Log(kDebug, "ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i) {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device, dev.function,
        vendor_id, class_code, dev.header_type);
  }

  // Intel 製を優先して xHC を探す
  pci::Device *xhc_dev = nullptr;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhc_dev = &pci::devices[i];

      if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
        break;
      }
    }
  }

  if (xhc_dev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device, xhc_dev->function);
  }

  SetIdtEntry(idt[InterruptVector::kXhci], MakeIdtAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXhci), kernel_cs);
  LoadIdt(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

  const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
  pci::ConfigureMsiFixedDestination(*xhc_dev, bsp_local_apic_id, pci::MsiTriggerMode::kLevel,
                                    pci::MsiDeliveryMode::kFixed, InterruptVector::kXhci, 0);

  const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
  Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
  const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
  Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

  usb::xhci::Controller xhc{xhc_mmio_base};

  if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
    SwitchEhci2Xhci(*xhc_dev);
  }
  {
    auto err = xhc.Initialize();
    Log(kDebug, "xhc.Initialize: %s\n", err.Name());
  }

  Log(kInfo, "xHC starting\n");
  xhc.Run();

  ::xhc = &xhc;

  usb::HIDMouseDriver::default_observer = MouseObserver;

  for (int i = 1; i <= xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

    if (port.IsConnected()) {
      if (auto err = ConfigurePort(xhc, port)) {
        Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(), err.File(), err.Line());
        continue;
      }
    }
  }

  const int kFrameWidth = frame_buffer_config.horizontal_resolution;
  const int kFrameHeight = frame_buffer_config.vertical_resolution;

  auto bgwindow =
      std::make_shared<Window>(kFrameWidth, kFrameHeight, frame_buffer_config.pixel_format);
  auto bgwriter = bgwindow->Writer();

  DrawDesktop(*bgwriter);
  console->SetWindow(bgwindow);

  auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight,
                                               frame_buffer_config.pixel_format);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});

  FrameBuffer screen;
  if (auto err = screen.Initialize(frame_buffer_config)) {
    Log(kError, "failed to initialize frame buffer: %s at %s:%d\n", err.Name(), err.File(),
        err.Line());
  }

  layer_manager = new LayerManager;
  layer_manager->SetWriter(&screen);

  auto bglayer_id = layer_manager->NewLayer().SetWindow(bgwindow).Move({0, 0}).Id();
  mouse_layer_id = layer_manager->NewLayer().SetWindow(mouse_window).Move({200, 200}).Id();

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(mouse_layer_id, 1);
  layer_manager->Draw();

  while (true) {
    __asm__("cli");
    if (main_queue.Count() == 0) {
      __asm__("sti\n\thlt");
      continue;
    }

    Message msg = main_queue.Front();
    main_queue.Pop();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXhci:
      while (xhc.PrimaryEventRing()->HasFront()) {
        if (auto err = ProcessEvent(xhc)) {
          Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(),
              err.Line());
        }
      }
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (1) {
    __asm__("hlt");
  }
}

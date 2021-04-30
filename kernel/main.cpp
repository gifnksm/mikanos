#include "asmfunc.h"
#include "console.hpp"
#include "elf.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>

const PixelColor kDesktopBgColor{45, 118, 237};
const PixelColor kDesktopFgColor{255, 255, 255};

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

  console->PutString(s);
  return result;
}

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
  mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

void SwitchEhci2Xhci(const pci::Device &xhc_dev) {
  bool intel_ehc_exist = false;
  for (uint32_t i = 0; i < pci::num_device; ++i) {
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
  Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n", superspeed_ports,
      ehci2xhci_ports);
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

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config) {
  switch (frame_buffer_config.pixel_format) {
  case kPixelRgbResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        RgbResv8BitPerColorPixelWriter(frame_buffer_config);
    break;
  case kPixelBgrResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf)
        BgrResv8BitPerColorPixelWriter(frame_buffer_config);
    break;
  }

  const uint32_t kFrameWidth = frame_buffer_config.horizontal_resolution;
  const uint32_t kFrameHeight = frame_buffer_config.vertical_resolution;

  FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50},
                kDesktopBgColor);
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50},
                {1, 8, 17});
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50},
                {80, 80, 80});
  DrawRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30},
                {160, 160, 160});

  console = new (console_buf)
      Console{*pixel_writer, kDesktopFgColor, kDesktopBgColor};
  printk("Welcome to MikanOS!\n");
  SetLogLevel(kWarn);

  mouse_cursor = new (mouse_cursor_buf)
      MouseCursor{pixel_writer, kDesktopBgColor, {300, 200}};

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue(main_queue_data);
  ::main_queue = &main_queue;

  auto err = pci::ScanAllBus();
  Log(kDebug, "ScanAllBus: %s\n", err.Name());

  for (uint32_t i = 0; i < pci::num_device; ++i) {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus,
        dev.device, dev.function, vendor_id, class_code, dev.header_type);
  }

  pci::Device *xhc_dev = nullptr;
  for (uint32_t i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhc_dev = &pci::devices[i];

      if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
        break;
      }
    }
  }

  if (xhc_dev != nullptr) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device,
        xhc_dev->function);
  }

  const uint16_t cs = GetCs();
  SetIdtEntry(idt[InterruptVector::kXhci],
              MakeIdtAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXhci), cs);
  LoadIdt(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

  const uint8_t bsp_local_apic_id =
      *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
  pci::ConfigureMsiFixedDestination(
      *xhc_dev, bsp_local_apic_id, pci::MsiTriggerMode::kLevel,
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

  for (uint32_t i = 1; i <= xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

    if (port.IsConnected()) {
      if (auto err = ConfigurePort(xhc, port)) {
        Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(),
            err.File(), err.Line());
        continue;
      }
    }
  }

  while (true) {
    __asm__("cli");
    if (main_queue.Count() == 0) {
      __asm__("sti\n\thlt");
    }

    Message msg = main_queue.Front();
    main_queue.Pop();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXhci:
      while (xhc.PrimaryEventRing()->HasFront()) {
        if (auto err = ProcessEvent(xhc)) {
          Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(),
              err.File(), err.Line());
        }
      }
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
}

extern "C" void __cxa_pure_virtual() {
  while (true) {
    __asm__("hlt");
  }
}

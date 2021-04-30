#include "console.hpp"
#include "elf.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>

void *operator new(size_t, void *buf) { return buf; }
void operator delete(void *) noexcept {}

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

  for (uint32_t x = 0; x < frame_buffer_config.horizontal_resolution; x++) {
    for (uint32_t y = 0; y < frame_buffer_config.vertical_resolution; y++) {
      pixel_writer->Write(x, y, {255, 255, 255});
    }
  }

  console =
      new (console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

  Console console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

  for (int i = 0; i < 27; ++i) {
    printk("printk: %d\n", i);
  }
  while (true) {
    __asm__("hlt");
  }
}

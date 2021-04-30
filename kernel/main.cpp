#include "elf.hpp"
#include "frame_buffer_config.hpp"
#include <cstddef>
#include <cstdint>

struct PixelColor {
  uint8_t r, g, b;
};

class PixelWriter {
public:
  PixelWriter(const FrameBufferConfig &config) : config_(config) {}

  virtual ~PixelWriter() = default;
  virtual void Write(uint32_t x, uint32_t y, const PixelColor &c) = 0;

protected:
  uint8_t *PixelAt(uint32_t x, uint32_t y) {
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig &config_;
};

class RgbResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(uint32_t x, uint32_t y, const PixelColor &c) override {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
};

class BgrResv8BitPerColorPixelWriter : public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  virtual void Write(uint32_t x, uint32_t y, const PixelColor &c) override {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  }
};

void *operator new(size_t, void *buf) { return buf; }
void operator delete(void *) noexcept {}

char pixel_writer_buf[sizeof(RgbResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

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
  for (uint32_t x = 0; x < 200; x++) {
    for (uint32_t y = 0; y < 100; y++) {
      pixel_writer->Write(x, y, {0, 255, 0});
    }
  }
  while (true) {
    __asm__("hlt");
  }
}

#include "graphics.hpp"

void RgbResv8BitPerColorPixelWriter::Write(uint32_t x, uint32_t y,
                                           const PixelColor &c) {
  auto p = PixelAt(x, y);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BgrResv8BitPerColorPixelWriter::Write(uint32_t x, uint32_t y,
                                           const PixelColor &c) {
  auto p = PixelAt(x, y);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

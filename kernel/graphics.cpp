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

void DrawRectangle(PixelWriter &writer, const Vector2D<uint32_t> &pos,
                   const Vector2D<uint32_t> &size, const PixelColor &c) {
  for (uint32_t dx = 0; dx < size.x; ++dx) {
    writer.Write(pos.x + dx, pos.y, c);
    writer.Write(pos.x + dx, pos.y + size.y - 1, c);
  }
  for (uint32_t dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos.x, pos.y + dy, c);
    writer.Write(pos.x + size.x - 1, pos.y + dy, c);
  }
}

void FillRectangle(PixelWriter &writer, const Vector2D<uint32_t> &pos,
                   const Vector2D<uint32_t> &size, const PixelColor &c) {
  for (uint32_t dy = 0; dy < size.y; ++dy) {
    for (uint32_t dx = 0; dx < size.x; ++dx) {
      writer.Write(pos.x + dx, pos.y + dy, c);
    }
  }
}

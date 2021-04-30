#include "font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t *GetFont(char c) {
  auto index = 16 * static_cast<unsigned int>(c);
  if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
    return nullptr;
  }
  return &_binary_hankaku_bin_start + index;
}

void WriteAscii(PixelWriter &writer, uint32_t x, uint32_t y, char c,
                const PixelColor &color) {
  const uint8_t *font = GetFont(c);
  if (font == nullptr) {
    return;
  }
  for (uint32_t dy = 0; dy < 16; dy++) {
    for (uint32_t dx = 0; dx < 8; dx++) {
      if (((font[dy] << dx) & 0x80u) != 0) {
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}

void WriteString(PixelWriter &writer, uint32_t x, uint32_t y, const char *s,
                 const PixelColor &color) {
  for (int i = 0; s[i] != '\0'; ++i) {
    WriteAscii(writer, x + 8 * i, y, s[i], color);
  }
}

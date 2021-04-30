#pragma once

#include "graphics.hpp"
#include <cstdint>

class Console {
public:
  static const int kRows = 25, kColumns = 80;

  Console(PixelWriter &writer, const PixelColor &fg_color,
          const PixelColor &bg_color);
  void PutString(const char *s);

private:
  void NewLine();

  PixelWriter &writer_;
  const PixelColor fg_color_, bg_color_;
  char buffer_[kRows][kColumns + 1];
  uint32_t cursor_row_, cursor_column_;
};

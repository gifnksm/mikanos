#pragma once

#include "graphics.hpp"
#include <cstdint>

void WriteAscii(PixelWriter &writer, uint32_t x, uint32_t y, char c,
                const PixelColor &color);
void WriteString(PixelWriter &writer, uint32_t x, uint32_t y, const char *s,
                 const PixelColor &color);

#pragma once

#include "error.hpp"
#include "graphics.hpp"

#include <cstdint>
#include <ft2build.h>
#include <utility>
#include FT_FREETYPE_H

void WriteAscii(PixelWriter &writer, Vector2D<int> pos, char c, const PixelColor &color);
void WriteString(PixelWriter &writer, Vector2D<int> pos, const char *s, const PixelColor &color);

int CountUtf8Size(uint8_t c);
std::pair<char32_t, int> ConvertUtf8To32(const char *u8);
bool IsHankaku(char32_t c);
WithError<FT_Face> NewFtFace();
Error WriteUnicode(PixelWriter &writer, Vector2D<int> pos, char32_t c, const PixelColor &color);
void InitializeFont();

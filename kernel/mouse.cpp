#include "mouse.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
// clang-format off
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};
// clang-format on

#include "logger.hpp"

void DrawMouseCursor(PixelWriter *pixel_writer, Vector2D<uint32_t> position) {
  for (uint32_t dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (uint32_t dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(position.x + dx, position.y + dy, {0, 0, 0});
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(position.x + dx, position.y + dy, {255, 255, 255});
      }
    }
  }
}

void EraseMouseCursor(PixelWriter *pixel_writer, Vector2D<uint32_t> position,
                      PixelColor erase_color) {
  for (uint32_t dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (uint32_t dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] != ' ') {
        pixel_writer->Write(position.x + dx, position.y + dy, erase_color);
      }
    }
  }
}

MouseCursor::MouseCursor(PixelWriter *writer, PixelColor erase_color,
                         Vector2D<uint32_t> initial_position)
    : pixel_writer_{writer}, erase_color_{erase_color}, position_{
                                                            initial_position} {
  DrawMouseCursor(pixel_writer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int32_t> displacement) {
  EraseMouseCursor(pixel_writer_, position_, erase_color_);
  position_ += displacement;
  DrawMouseCursor(pixel_writer_, position_);
}

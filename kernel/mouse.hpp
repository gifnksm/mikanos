#pragma once

#include "graphics.hpp"

class MouseCursor {
public:
  MouseCursor(PixelWriter *writer, PixelColor erase_color,
              Vector2D<uint32_t> initial_position);
  void MoveRelative(Vector2D<int32_t> displacement);

private:
  PixelWriter *pixel_writer_ = nullptr;
  PixelColor erase_color_;
  Vector2D<uint32_t> position_;
};

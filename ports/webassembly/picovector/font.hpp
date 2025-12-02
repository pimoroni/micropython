#pragma once

#include <stdint.h>
#include <string>

#include "brush.hpp"
#include "shape.hpp"
#include "rect.hpp"
#include "point.hpp"
#include "matrix.hpp"

namespace picovector {

  class image_t;

  class glyph_path_point_t {
  public:
    int8_t x, y;
    
    point_t transform(mat3_t *transform);
  };

  class glyph_path_t {
  public:
    uint16_t point_count;
    glyph_path_point_t *points;
  };

  class glyph_t {
  public:
    uint16_t codepoint;
    int8_t x, y, w, h;
    int8_t advance;
    uint8_t path_count;
    glyph_path_t *paths;   
    
    rect_t bounds(mat3_t *transform);
  };

  class font_t {
  public:
    int glyph_count;
    glyph_t *glyphs;

    void draw(image_t *target, const char *text, float x, float y, float size);
    rect_t measure(image_t *target, const char *text, float size);
  };

}
#pragma once

#include <stdint.h>
#include <map>
#include <string>

#include "brush.hpp"
#include "shape.hpp"
#include "types.hpp"
#include "mat3.hpp"

using std::vector;
using std::pair;
using std::less;

namespace picovector {

  class image_t;

  struct pixel_font_glyph_t {
    uint32_t codepoint;
    uint16_t width;
  };

  class pixel_font_t {
  public:
    uint32_t glyph_count;
    uint32_t glyph_data_size;
    uint16_t width;
    uint16_t height;
    char name[32];

    pixel_font_glyph_t *glyphs;
    uint8_t *glyph_data;

    int glyph_index(int codepoint);

    void draw(image_t *target, const char *text, int x, int y);
    void draw_glyph(image_t *target, const pixel_font_glyph_t *glyph, uint8_t *data, brush_t *brush, const rect_t &bounds, int x, int y);
    rect_t measure(image_t *target, const char *text);
  };

}
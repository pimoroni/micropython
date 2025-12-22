#pragma once

#include "picovector.hpp"
#include "mat3.hpp"
#include "image.hpp"

namespace picovector {

  typedef void (*pixel_func_t)(brush_t *brush, int x, int y);
  typedef void (*span_func_t)(brush_t *brush, int x, int y, int w);
  typedef void (*mask_span_func_t)(brush_t *brush, int x, int y, int w, uint8_t *mask);
  struct brush_t {
    pixel_func_t pixel_func;
    span_func_t span_func;
    mask_span_func_t mask_span_func;
    image_t *target;

    explicit brush_t(image_t *t) : target(t) {}
  };

  struct color_brush_t : public brush_t {
    uint32_t c;
    uint8_t r, g, b, a;

    color_brush_t(image_t *target, uint32_t c);
  };

  extern const uint8_t patterns[38][8];
  struct pattern_brush_t : public brush_t {
  public:
    uint8_t p[8];
    uint32_t c1;
    uint32_t c2;

    pattern_brush_t(image_t *target, uint32_t c1, uint32_t c2, uint8_t pattern_index);
    pattern_brush_t(image_t *target, uint32_t c1, uint32_t c2, uint8_t *pattern);
  };

  struct image_brush_t : public brush_t {
  public:
    image_t *src;
    mat3_t inverse_transform;

    image_brush_t(image_t *target, image_t *src);
    image_brush_t(image_t *target, image_t *src, mat3_t *transform);
  };

  // class brighten_brush : public brush_t {
  // public:
  //   int amount;
  //   brighten_brush(int amount) : amount(amount) {}
  //   void render_span(image_t *target, int x, int y, int w);
  //   void render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb) {};
  // };

  // class xor_brush : public brush_t {
  // public:
  //   uint32_t color;
  //   xor_brush(uint32_t color) : color(color) {}
  //   void render_span(image_t *target, int x, int y, int w);
  //   void render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb);
  // };

}
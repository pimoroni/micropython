#pragma once

#include "picovector.hpp"
#include "mat3.hpp"
#include "image.hpp"
#include "blend.hpp"
#include "color.hpp"

namespace picovector {

  class brush_t {
  public:
    virtual span_func_t span_func() = 0;
    virtual masked_span_func_t masked_span_func() = 0;
  };

  void color_brush_span_func(image_t *target, brush_t *brush, int x, int y, int w);
  void color_brush_masked_span_func(image_t *target, brush_t *brush, int x, int y, int w, uint8_t *mask);
  class color_brush_t : public brush_t {
  public:
    color_t c;

    color_brush_t(const color_t& c);
    span_func_t span_func();
    masked_span_func_t masked_span_func();
  };

  class pattern_brush_t : public brush_t {
  public:
    uint8_t p[8];
    color_t c1;
    color_t c2;

    pattern_brush_t(const color_t& c1, const color_t& c2, uint8_t pattern_index);
    pattern_brush_t(const color_t& c1, const color_t& c2, uint8_t *pattern);
    span_func_t span_func();
    masked_span_func_t masked_span_func();
  };

  class image_brush_t : public brush_t {
  public:
    image_t *src;
    mat3_t inverse_transform;

    image_brush_t(image_t *src);
    image_brush_t(image_t *src, mat3_t *transform);
    span_func_t span_func();
    masked_span_func_t masked_span_func();
  };

}
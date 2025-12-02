#include "brush.hpp"
#include "image.hpp"
#include "blend.hpp"

//using namespace std;

namespace picovector {

  #define debug_printf(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)


  void brush_t::render_spans(image_t *target, _rspan *spans, int count) {
    while(count--) {
      this->render_span(target, spans->x, spans->y, spans->w);
      spans++;
    }
  }

  color_brush::color_brush(int r, int g, int b, int a) {
    this->color = _make_col(r, g, b, a);
  }

  void color_brush::pixel(uint32_t *dst) {
    _blend_rgba_rgba((uint8_t*)dst, (uint8_t*)&color);
  }

  void color_brush::render_span(image_t *target, int x, int y, int w) {
    _span_blend_rgba_rgba((uint8_t*)target->ptr(x, y), (uint8_t*)&color, w);
  }

  void color_brush::render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb) {
    _span_blend_rgba_rgba_masked((uint8_t*)target->ptr(x, y), (uint8_t*)&color, sb, w);
  }

  brighten_brush::brighten_brush(int amount) : amount(amount) {}

  void brighten_brush::pixel(uint32_t *dst) {
    return;
  }

  void brighten_brush::render_span(image_t *target, int x, int y, int w) {
    uint32_t *dst = (uint32_t*)target->ptr(x, y);

    while(w--) {
      uint8_t *pd = (uint8_t *)dst;

      int a = (pd[3] * amount) >> 8;

      int r = pd[0] + a;
      r = max(0, min(r, 255));
      pd[0] = r;

      int g = pd[1] + a;
      g = max(0, min(g, 255));
      pd[1] = g;

      int b = pd[2] + a;
      b = max(0, min(b, 255));
      pd[2] = b;

      dst++;
    }
  }

  xor_brush::xor_brush(int r, int g, int b) {
    this->color = _make_col(r, g, b);
  }

  void xor_brush::pixel(uint32_t *dst) {
    return;
  }

  void xor_brush::render_span(image_t *target, int x, int y, int w) {
    uint8_t *dst = (uint8_t*)target->ptr(x, y);
    uint8_t *src = (uint8_t*)&color;
    while(w--) {
      uint32_t xored = _make_col(dst[0] ^ src[0], dst[1] ^ src[1], dst[2] ^ src[2], src[3]);
      _blend_rgba_rgba(dst, (uint8_t*)&xored);
      dst += 4;
    }
  }


  void xor_brush::render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb) {
    uint8_t *dst = (uint8_t*)target->ptr(x, y);
    uint8_t *src = (uint8_t*)&color;
    while(w--) {
      uint32_t xored = _make_col(dst[0] ^ src[0], dst[1] ^ src[1], dst[2] ^ src[2], *sb);
      _blend_rgba_rgba(dst, (uint8_t*)&xored);
      dst += 4;
      sb++;
    }

    //uint32_t *dst = target->ptr(x, y);
    //span_argb8(dst, w, color, sb);
  }


  // void xor::render_spans(image *target, shape *shape, render_span *spans, int count) {
  //   while(count--) {
  //     debug_printf("%d, %d (%d)\n", spans->x, spans->y, spans->w);

  //     uint32_t *dst = target->ptr(spans->x, spans->y);
  //     for(int i = 0; i < spans->w; i++) {
  //       uint8_t *pd = (uint8_t *)dst;
  //       pd[1] = ^pd[1];
  //       pd[2] = ^pd[2];
  //       pd[3] = ^pd[3];

  //       dst++;
  //     }
  //     spans++;
  //   }
  // }
}
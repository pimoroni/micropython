#include "../brush.hpp"

namespace picovector {

  void image_brush_span_func(image_t *target, brush_t *brush, int x, int y, int w) {
    image_brush_t *p = (image_brush_t*)brush;
    uint32_t *dst = (uint32_t*)target->ptr(x, y);
    rect_t b = p->src->bounds();

    fx16_vec2_t p1(x, y);
    fx16_vec2_t p2((x + w), y);

    p1 = p1.transform(&p->inverse_transform);
    p2 = p2.transform(&p->inverse_transform);

    fx16_vec2_t pd((p2.x - p1.x) / w, (p2.y - p1.y) / w);
    fx16_vec2_t pt = p1;

    int tw = int(b.w);
    int th = int(b.h);

    for(int i = 0; i < w; i++) {
      pt.x += pd.x;
      pt.y += pd.y;
      int u = ((int(pt.x) >> 16) % tw + tw) % tw;
      int v = ((int(pt.y) >> 16) % th + th) % th;
      uint32_t c = p->src->get_unsafe(u, v);
      uint8_t *src = (uint8_t*)&c;
      *dst = target->_blend_func(*dst, src[0], src[1], src[2], src[3]);
      dst++;
    }
  }

  void image_brush_masked_span_func(image_t *target, brush_t *brush, int x, int y, int w, uint8_t *mask) {
    image_brush_t *p = (image_brush_t*)brush;
    uint32_t *dst = (uint32_t*)target->ptr(x, y);
    rect_t b = p->src->bounds();

    fx16_vec2_t p1(x, y);
    fx16_vec2_t p2((x + w), y);

    p1 = p1.transform(&p->inverse_transform);
    p2 = p2.transform(&p->inverse_transform);

    fx16_vec2_t pd((p2.x - p1.x) / w, (p2.y - p1.y) / w);
    fx16_vec2_t pt = p1;

    int tw = int(b.w);
    int th = int(b.h);

    for(int i = 0; i < w; i++) {
      pt.x += pd.x;
      pt.y += pd.y;
      int u = ((int(pt.x) >> 16) % tw + tw) % tw;
      int v = ((int(pt.y) >> 16) % th + th) % th;
      uint32_t c = p->src->get_unsafe(u, v);
      uint8_t *src = (uint8_t*)&c;
      uint32_t m = *mask;
      uint32_t sr = (src[0] * m + 128) >> 8;
      uint32_t sg = (src[1] * m + 128) >> 8;
      uint32_t sb = (src[2] * m + 128) >> 8;
      uint32_t sa = (src[3] * m + 128) >> 8;

      *dst = target->_blend_func(*dst, sr, sg, sb, sa);
      dst++;
      mask++;
    }
  }

  image_brush_t::image_brush_t(image_t *src) : src(src) {
  }

  image_brush_t::image_brush_t(image_t *src, mat3_t *transform) : src(src) {
    if(transform) {
      inverse_transform = *transform;
      inverse_transform.inverse();
    }
  }

  span_func_t image_brush_t::span_func() {
    return image_brush_span_func;
  }

  masked_span_func_t image_brush_t::masked_span_func() {
    return image_brush_masked_span_func;
  }
}
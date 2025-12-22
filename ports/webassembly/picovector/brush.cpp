#include "types.hpp"
#include "blend.hpp"

#include "brush.hpp"

namespace picovector {

  // empty implementations for unsupported modes
  void pixel_func_nop(brush_t *brush, int x, int y) {}
  void span_func_nop(brush_t *brush, int x, int y, int w) {}

  // copies the source color over the destination colour, ignoring alpha
  void color_pixel_func_rgba_rgba_copy(brush_t *brush, int x, int y) {
    color_brush_t *p = (color_brush_t*)brush;
    uint32_t *dst = (uint32_t*)brush->target->ptr(x, y);
    *dst = p->c;
  }
  void color_span_func_rgba_rgba_copy(brush_t *brush, int x, int y, int w) {
    color_brush_t *p = (color_brush_t*)brush;
    uint32_t *dst = (uint32_t*)brush->target->ptr(x, y);
    int dw8 = w >> 3;   // number of blocks of eight pixels
    int r = w & 0b111;  // remainder
    uint32_t c = p->c;
    while(dw8--) { // unrolled blocks of 8 pixels
      *dst++ = c; *dst++ = c; *dst++ = c; *dst++ = c;
      *dst++ = c; *dst++ = c; *dst++ = c; *dst++ = c;
    }
    while(r--) { // fill in remainder
      *dst++ = c;
    }
  }

  // blend the source colour over the destination colour
  void color_pixel_func_rgba_rgba_blend(brush_t *brush, int x, int y) {
    color_brush_t *p = (color_brush_t*)brush;
    blend_rgba_rgba((uint8_t*)brush->target->ptr(x, y), p->r, p->g, p->b, p->a);
  }
  void color_span_func_rgba_rgba_blend(brush_t *brush, int x, int y, int w) {
    color_brush_t *p = (color_brush_t*)brush;
    span_blend_rgba_rgba((uint8_t*)brush->target->ptr(x, y), (uint8_t*)&p->c, w);
  }
  void color_mask_span_func_rgba_rgba_blend(brush_t *brush,  int x, int y, int w, uint8_t *mask) {
    color_brush_t *p = (color_brush_t*)brush;
    mask_span_blend_rgba_rgba((uint8_t*)brush->target->ptr(x, y), (uint8_t*)&p->c, w, mask);
  }

  color_brush_t::color_brush_t(image_t *target, uint32_t c) : brush_t(target), c(c) {
    if(target->has_palette()) {
      assert(false && "color brush on paletted image is not supported");
      pixel_func = pixel_func_nop;
      span_func = span_func_nop;
    }else{
      r = get_r(&c);
      g = get_g(&c);
      b = get_b(&c);
      a = get_a(&c);
      if(a == 255) {
        // source is opaque, use faster copy function
        pixel_func = color_pixel_func_rgba_rgba_copy;
        span_func = color_span_func_rgba_rgba_copy;
        mask_span_func = color_mask_span_func_rgba_rgba_blend;
      }else{
        pixel_func = color_pixel_func_rgba_rgba_blend;
        span_func = color_span_func_rgba_rgba_blend;
        mask_span_func = color_mask_span_func_rgba_rgba_blend;
      }
    }
  }

  // blend the source colour over the destination colour
  void pattern_pixel_func_rgba_rgba_blend(brush_t *brush, int x, int y) {
    pattern_brush_t *p = (pattern_brush_t*)brush;
    uint8_t *dst = (uint8_t*)brush->target->ptr(x, y);
    uint8_t u = 7 - (x & 0b111);
    uint8_t v = y & 0b111;
    uint8_t b = p->p[v];
    uint8_t *src = b & (1 << u) ? (uint8_t*)&p->c1 : (uint8_t*)&p->c2;
    blend_rgba_rgba(dst, src[0], src[1], src[2], src[3]);
  }

  void pattern_span_func_rgba_rgba_blend(brush_t *brush, int x, int y, int w) {
    pattern_brush_t *p = (pattern_brush_t*)brush;
    uint8_t *dst = (uint8_t*)brush->target->ptr(x, y);
    while(w--) {
      uint8_t u = 7 - (x & 0b111);
      uint8_t v = y & 0b111;
      uint8_t b = p->p[v];
      uint8_t *src = b & (1 << u) ? (uint8_t*)&p->c1 : (uint8_t*)&p->c2;
      blend_rgba_rgba(dst, src[0], src[1], src[2], src[3]);
      dst += 4;
      x++;
    }
  }
  void pattern_mask_span_func_rgba_rgba_blend(brush_t *brush, int x, int y, int w, uint8_t *mask) {
    pattern_brush_t *p = (pattern_brush_t*)brush;
    uint8_t *dst = (uint8_t*)brush->target->ptr(x, y);
    while(w--) {
      uint8_t u = 7 - (x & 0b111);
      uint8_t v = y & 0b111;
      uint8_t b = p->p[v];
      uint8_t *src = b & (1 << u) ? (uint8_t*)&p->c1 : (uint8_t*)&p->c2;
      uint16_t t = *mask * src[3] + 128; // combine source alpha with mask alpha
      uint8_t ma = (t + (t >> 8)) >> 8;
      blend_rgba_rgba(dst, src[0], src[1], src[2], ma);
      dst += 4;
      x++;
      mask++;
    }
  }

  pattern_brush_t::pattern_brush_t(image_t *target, uint32_t c1, uint32_t c2, uint8_t pattern_index) : brush_t(target), c1(c1), c2(c2) {
    memcpy(this->p, &patterns[pattern_index], sizeof(uint8_t) * 8);

    if(target->has_palette()) {
      assert(false && "pattern brush on paletted image is not supported");
      pixel_func = pixel_func_nop;
      span_func = span_func_nop;
    }else{
      pixel_func = pattern_pixel_func_rgba_rgba_blend;
      span_func = pattern_span_func_rgba_rgba_blend;
      mask_span_func = pattern_mask_span_func_rgba_rgba_blend;
    }
  }

  pattern_brush_t::pattern_brush_t(image_t *target, uint32_t c1, uint32_t c2, uint8_t *pattern) : brush_t(target), c1(c1), c2(c2) {
    memcpy(this->p, pattern, sizeof(uint8_t) * 8);

    if(target->has_palette()) {
      assert(false && "pattern brush on paletted image is not supported");
      pixel_func = pixel_func_nop;
      span_func = span_func_nop;
    }else{
      pixel_func = pattern_pixel_func_rgba_rgba_blend;
      span_func = pattern_span_func_rgba_rgba_blend;
      mask_span_func = pattern_mask_span_func_rgba_rgba_blend;
    }
  }

  // blend the source colour over the destination colour
  void image_pixel_func_rgba_rgba_blend(brush_t *brush, int x, int y) {
    // currently a NOP
  }

  void image_span_func_rgba_rgba_blend(brush_t *brush, int x, int y, int w) {
    image_brush_t *p = (image_brush_t*)brush;
    uint8_t *dst = (uint8_t*)brush->target->ptr(x, y);
    rect_t b = p->src->bounds();

    fx16_point_t p1(x, y);
    fx16_point_t p2((x + w), y);

    p1 = p1.transform(&p->inverse_transform);
    p2 = p2.transform(&p->inverse_transform);

    fx16_point_t pd((p2.x - p1.x) / w, (p2.y - p1.y) / w);
    fx16_point_t pt = p1;

    int tw = int(b.w);
    int th = int(b.h);

    for(int i = 0; i < w; i++) {
      pt.x += pd.x;
      pt.y += pd.y;
      int u = ((int(pt.x) >> 16) % tw + tw) % tw;
      int v = ((int(pt.y) >> 16) % th + th) % th;
      uint32_t c = p->src->get_unsafe(u, v);
      uint8_t *src = (uint8_t*)&c;
      blend_rgba_rgba(dst, src[0], src[1], src[2], src[3]);
      dst += 4;
    }
  }

  void image_mask_span_func_rgba_rgba_blend(brush_t *brush, int x, int y, int w, uint8_t *mask) {
    image_brush_t *p = (image_brush_t*)brush;
    uint8_t *dst = (uint8_t*)brush->target->ptr(x, y);
    rect_t b = p->src->bounds();

    fx16_point_t p1(x, y);
    fx16_point_t p2((x + w), y);

    p1 = p1.transform(&p->inverse_transform);
    p2 = p2.transform(&p->inverse_transform);

    fx16_point_t pd((p2.x - p1.x) / w, (p2.y - p1.y) / w);
    fx16_point_t pt = p1;

    int tw = int(b.w);
    int th = int(b.h);

    for(int i = 0; i < w; i++) {
      pt.x += pd.x;
      pt.y += pd.y;
      int u = ((int(pt.x) >> 16) % tw + tw) % tw;
      int v = ((int(pt.y) >> 16) % th + th) % th;
      uint32_t c = p->src->get_unsafe(u, v);
      uint8_t *src = (uint8_t*)&c;
      uint16_t t = *mask * src[3] + 128; // combine source alpha with mask alpha
      uint8_t ma = (t + (t >> 8)) >> 8;
      blend_rgba_rgba(dst, src[0], src[1], src[2], ma);
      dst += 4;
      mask++;
    }
  }

  image_brush_t::image_brush_t(image_t *target, image_t *src) : brush_t(target), src(src) {
    if(target->has_palette()) {
      assert(false && "image brush on paletted image is not supported");
      pixel_func = pixel_func_nop;
      span_func = span_func_nop;
    }else{
      pixel_func = image_pixel_func_rgba_rgba_blend;
      span_func = image_span_func_rgba_rgba_blend;
      mask_span_func = image_mask_span_func_rgba_rgba_blend;
    }
  }

  image_brush_t::image_brush_t(image_t *target, image_t *src, mat3_t *transform) : brush_t(target), src(src) {
    if(transform) {
      inverse_transform = *transform;
      inverse_transform.inverse();
    }

    if(target->has_palette()) {
      assert(false && "image brush on paletted image is not supported");
      pixel_func = pixel_func_nop;
      span_func = span_func_nop;
    }else{
      pixel_func = image_pixel_func_rgba_rgba_blend;
      span_func = image_span_func_rgba_rgba_blend;
      mask_span_func = image_mask_span_func_rgba_rgba_blend;
    }
  }


  // embedded patterns
  const uint8_t patterns[38][8] = {
    {0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
    {0b00100010,0b00000000,0b10001000,0b00000000,0b00100010,0b00000000,0b10001000,0b00000000},
    {0b00100010,0b10001000,0b00100010,0b10001000,0b00100010,0b10001000,0b00100010,0b10001000},
    {0b01010101,0b10101010,0b01010101,0b10101010,0b01010101,0b10101010,0b01010101,0b10101010},
    {0b10101010,0b00000000,0b10101010,0b00000000,0b10101010,0b00000000,0b10101010,0b00000000},
    {0b01010101,0b01010101,0b01010101,0b01010101,0b01010101,0b01010101,0b01010101,0b01010101},
    {0b00010001,0b00100010,0b01000100,0b10001000,0b00010001,0b00100010,0b01000100,0b10001000},
    {0b01110111,0b01110111,0b01110111,0b01110111,0b01110111,0b01110111,0b01110111,0b01110111},
    {0b01001110,0b11001111,0b11111100,0b11100100,0b00100111,0b00111111,0b11110011,0b01110010},
    {0b01111111,0b11101111,0b11111101,0b11011111,0b11111110,0b11110111,0b10111111,0b11111011},
    {0b00000000,0b01110111,0b01110111,0b01110111,0b00000000,0b01110111,0b01110111,0b01110111},
    {0b00000000,0b01111111,0b01111111,0b01111111,0b00000000,0b11110111,0b11110111,0b11110111},
    {0b01111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111},
    {0b01111111,0b10111111,0b11011111,0b11111111,0b11111101,0b11111011,0b11110111,0b11111111},
    {0b01111101,0b10111011,0b11000110,0b10111011,0b01111101,0b11111110,0b11111110,0b11111110},
    {0b00000111,0b10001011,0b11011101,0b10111000,0b01110000,0b11101000,0b11011101,0b10001110},
    {0b10101010,0b01011111,0b10111111,0b10111111,0b10101010,0b11110101,0b11111011,0b11111011},
    {0b11011111,0b10101111,0b01110111,0b01110111,0b01110111,0b01110111,0b11111010,0b11111101},
    {0b01000000,0b11111111,0b01000000,0b01000000,0b01001111,0b01001111,0b01001111,0b01001111},
    {0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111111},
    {0b01111111,0b11111111,0b11110111,0b11111111,0b01111111,0b11111111,0b11110111,0b11111111},
    {0b01110111,0b11111111,0b11011101,0b11111111,0b01110111,0b11111111,0b11011101,0b11111111},
    {0b01110111,0b11011101,0b01110111,0b11011101,0b01110111,0b11011101,0b01110111,0b11011101},
    {0b01010101,0b11111111,0b01010101,0b11111111,0b01010101,0b11111111,0b01010101,0b11111111},
    {0b00000000,0b11111111,0b00000000,0b11111111,0b00000000,0b11111111,0b00000000,0b11111111},
    {0b11101110,0b11011101,0b10111011,0b01110111,0b11101110,0b11011101,0b10111011,0b01110111},
    {0b00000000,0b11111111,0b11111111,0b11111111,0b00000000,0b11111111,0b11111111,0b11111111},
    {0b11111110,0b11111101,0b11111011,0b11110111,0b11101111,0b11011111,0b10111111,0b01111111},
    {0b01010101,0b11111111,0b01111111,0b11111111,0b01110111,0b11111111,0b01111111,0b11111111},
    {0b00000000,0b01111111,0b01111111,0b01111111,0b01111111,0b01111111,0b01111111,0b01111111},
    {0b11110111,0b11100011,0b11011101,0b00111110,0b01111111,0b11111110,0b11111101,0b11111011},
    {0b01110111,0b11101011,0b11011101,0b10111110,0b01110111,0b11111111,0b01010101,0b11111111},
    {0b10111111,0b01011111,0b11111111,0b11111111,0b11111011,0b11110101,0b11111111,0b11111111},
    {0b11111100,0b01111011,0b10110111,0b11001111,0b11110011,0b11111101,0b11111110,0b11111110},
    {0b01111111,0b01111111,0b10111110,0b11000001,0b11110111,0b11110111,0b11101011,0b00011100},
    {0b11101111,0b11011111,0b10101011,0b01010101,0b00000000,0b11111101,0b11111011,0b11110111},
    {0b10001000,0b01110110,0b01110000,0b01110000,0b10001000,0b01100111,0b00000111,0b00000111},
    {0b11111111,0b11110111,0b11101011,0b11010101,0b10101010,0b11010101,0b11101011,0b11110111}
  };

  // void color_brush::render_span(image_t *target, int x, int y, int w) {
  //   _span_blend_rgba_rgba((uint8_t*)target->ptr(x, y), (uint8_t*)&color, w);
  // }

  // void color_brush::render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb) {
  //   _span_blend_rgba_rgba_masked((uint8_t*)target->ptr(x, y), (uint8_t*)&color, sb, w);
  // }




  // void brighten_brush::render_span(image_t *target, int x, int y, int w) {
  //   uint32_t *dst = (uint32_t*)target->ptr(x, y);

  //   while(w--) {
  //     uint8_t *pd = (uint8_t *)dst;

  //     int a = (pd[3] * amount) >> 8;

  //     int r = pd[0] + a;
  //     r = max(0, min(r, 255));
  //     pd[0] = r;

  //     int g = pd[1] + a;
  //     g = max(0, min(g, 255));
  //     pd[1] = g;

  //     int b = pd[2] + a;
  //     b = max(0, min(b, 255));
  //     pd[2] = b;

  //     dst++;
  //   }
  // }

  // void xor_brush::render_span(image_t *target, int x, int y, int w) {
  //   uint8_t *dst = (uint8_t*)target->ptr(x, y);
  //   uint8_t *src = (uint8_t*)&color;
  //   uint8_t sr = src[0], sg = src[1], sb = src[2], sa = src[3];
  //   while(w--) {
  //     _blend_rgba_rgba(dst, dst[0] ^ sr, dst[1] ^ sg, dst[2] ^ sb, sa);
  //     dst += 4;
  //   }
  // }


  // void xor_brush::render_span_buffer(image_t *target, int x, int y, int w, uint8_t *sb) {
  //   uint8_t *dst = (uint8_t*)target->ptr(x, y);
  //   uint8_t *src = (uint8_t*)&color;
  //   uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
  //   while(w--) {
  //     _blend_rgba_rgba(dst, dst[0] ^ r, dst[1] ^ g, dst[2] ^ b, *sb);
  //     dst += 4;
  //     sb++;
  //   }
  // }

}
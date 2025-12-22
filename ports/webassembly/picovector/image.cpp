#include <string.h>
#include <math.h>
#include <algorithm>
#include <vector>

#include "rasteriser.hpp"
#include "algorithms/algorithms.hpp"
#include "image.hpp"
#include "blend.hpp"
#include "brush.hpp"
#include "shape.hpp"

using std::vector;

namespace picovector {

  image_t::image_t() {
  }

  image_t::image_t(image_t *source, rect_t r) {
    rect_t i = source->_bounds.intersection(r);
    *this = *source;
    _bounds = rect_t(0, 0, i.w, i.h);
    _clip = rect_t(0, 0, i.w, i.h);
    _buffer = source->ptr(i.x, i.y);
    _managed_buffer = false;
  }

  image_t::image_t(int w, int h, pixel_format_t pixel_format, bool has_palette) {
    _bounds = rect_t(0, 0, w, h);
    _clip = rect_t(0, 0, w, h);
    _brush = nullptr;
    _pixel_format = pixel_format;
    _has_palette = has_palette;
    _managed_buffer = true;
    _bytes_per_pixel = this->_has_palette ? sizeof(uint8_t) : sizeof(uint32_t);
    _row_stride = w * _bytes_per_pixel;
    _buffer = PV_MALLOC(this->buffer_size());
    if(_has_palette) {
      _palette.resize(256);
    }
  }

  image_t::image_t(void *buffer, int w, int h, pixel_format_t pixel_format, bool has_palette) {
    _bounds = rect_t(0, 0, w, h);
    _clip = rect_t(0, 0, w, h);
    _brush = nullptr;
    _pixel_format = pixel_format;
    _has_palette = has_palette;
    _buffer = buffer;
    _managed_buffer = false;
    _bytes_per_pixel = this->_has_palette ? sizeof(uint8_t) : sizeof(uint32_t);
    _row_stride = w * _bytes_per_pixel;
    if(_has_palette) {
      _palette.resize(256);
    }
  }

  image_t::~image_t() {
    if(this->_managed_buffer) {
#ifdef PICO
      PV_FREE(this->_buffer);
#else
      PV_FREE(this->_buffer, this->buffer_size());
#endif
    }
  }

  size_t image_t::buffer_size() {
    return this->_bytes_per_pixel * this->_bounds.w * this->_bounds.h;
  }

  size_t image_t::bytes_per_pixel() {
    return this->_bytes_per_pixel;
  }

  bool image_t::is_compatible(image_t *other) {
    return this->_palette == other->_palette && this->_pixel_format == other->_pixel_format;
  }

  uint32_t image_t::row_stride() {
    return this->_row_stride;
  }

  rect_t image_t::bounds() {
    return this->_bounds;
  }

  rect_t image_t::clip() {
    return this->_clip;
  }

  void image_t::clip(rect_t r) {
    this->_clip = _bounds.intersection(r);
  }

  bool image_t::has_palette() {
    return this->_has_palette;
  }

  // TODO: why?
  // void image_t::delete_palette() {
  //   if(this->has_palette()) {
  //     this->_palette.clear();
  //   }
  // }

  void image_t::palette(uint8_t i, uint32_t c) {
    this->_palette[i] = c;
  }

  uint32_t image_t::palette(uint8_t i) {
    return this->_palette[i];
  }

  uint8_t image_t::alpha() {
    return this->_alpha;
  }

  void image_t::alpha(uint8_t alpha) {
    // TODO: check if pixel format and palette mode supports alpha
    this->_alpha = alpha;
  }

  antialias_t image_t::antialias() {
    return this->_antialias;
  }

  void image_t::antialias(antialias_t antialias) {
    // TODO: check if pixel format and palette mode supports alpha
    this->_antialias = antialias;
  }

  pixel_format_t image_t::pixel_format() {
    return this->_pixel_format;
  }

  void image_t::pixel_format(pixel_format_t pixel_format) {
    this->_pixel_format = pixel_format;
  }

  brush_t* image_t::brush() {
    return this->_brush;
  }

  void image_t::brush(brush_t *brush) {
    this->_brush = brush;
  }

  font_t* image_t::font() {
    return this->_font;
  }

  void image_t::font(font_t *font) {
    this->_font = font;
  }

  pixel_font_t* image_t::pixel_font() {
    return this->_pixel_font;
  }

  void image_t::pixel_font(pixel_font_t *pixel_font) {
    this->_pixel_font = pixel_font;
  }

  image_t image_t::window(rect_t r) {
    rect_t i = _bounds.intersection(r);
    image_t window = image_t(this, rect_t(i.x, i.y, i.w, i.h));
    return window;
  }

  // void image_t::clear(uint32_t c) {
  //   int count = this->_bounds.w * this->_bounds.h;

  //   printf("clear %p (%d)\n", this->_brush, count);
  //   this->_brush->span_func(this->_brush, 0, 0, count);

  //   // if(_has_palette) {
  //   //   memset(_buffer, c, count);
  //   // }else{
  //   //   int dw8 = count >> 3;   // number of blocks of eight pixels
  //   //   int r = count & 0b111;  // remainder
  //   //   uint32_t* p = (uint32_t*)_buffer;
  //   //   while(dw8--) { // unrolled blocks of 8 pixels
  //   //     *p++ = c; *p++ = c; *p++ = c; *p++ = c;
  //   //     *p++ = c; *p++ = c; *p++ = c; *p++ = c;
  //   //   }
  //   //   while(r--) { // fill in remainder
  //   //     *p++ = c;
  //   //   }
  //   // }
  // }

  void image_t::clear() {
    int count = this->_bounds.w * this->_bounds.h;
    this->_brush->span_func(this->_brush, 0, 0, count);

//    rectangle(_clip);
  }

  void image_t::blit(image_t *t, const point_t p) {
    rect_t tr(p.x, p.y, _bounds.w, _bounds.h); // target rect

    int yoff = tr.y < t->_clip.y ? t->_clip.y - tr.y : 0;
    int xoff = tr.x < t->_clip.x ? t->_clip.x - tr.x : 0;

    tr = tr.intersection(t->_clip); // clip to target image bounds

    if(tr.empty()) {return;}

    int sxo = xoff;
    int syo = yoff;// p.y < 0 ? -p.y : 0;

    for(int i = 0; i < tr.h; i++) {
      uint8_t *dst = (uint8_t *)t->ptr(tr.x, tr.y + i);
      uint8_t *src = (uint8_t *)this->ptr(sxo, syo + i);
      if(this->_has_palette) {
        _span_blit_rgba_rgba(dst, src, (uint8_t*)&this->_palette[0], tr.w, this->_alpha);
      }else{
        _span_blit_rgba_rgba(dst, src, tr.w, this->_alpha);
      }
    }
  }

  /*
    renders a vertical span onto the target image using this image as a
    texture.

    - p: the starting point of the span on the target
    - c: the count of pixels to render
    - uvs: the start coordinate of the texture
    - uve: the end coordinate of the texture
  */
  void image_t::vspan_tex(image_t *target, point_t p, uint c, point_t uvs, point_t uve) {
    rect_t b = target->_clip;
    if(p.x < b.x || p.x > b.x + b.w) {
      return;
    }

    float ustep = (uve.x - uvs.x) / float(c);
    float vstep = (uve.y - uvs.y) / float(c);
    float u = uvs.x;
    float v = uvs.y;

    for(int y = p.y; y < p.y + c; y++) {
      u += ustep;
      v += vstep;

      if(y >= b.y && y < b.y + b.h) {
        uint8_t *dst = (uint8_t *)target->ptr(p.x, y);

        int tx = round(u);
        int ty = round(v);

        uint32_t col;
        uint8_t *src;
        if(this->_has_palette) {
          src = (uint8_t *)&this->_palette[*(uint8_t *)this->ptr(tx, ty)];
        } else {
          src = (uint8_t *)this->ptr(tx, ty);
        }

        blend_rgba_rgba(dst, src[0], src[1], src[2], src[3]);
      }
    }
  }

  // blit from source rectangle into target rectangle
  void image_t::blit(image_t *t, rect_t sr, rect_t tr) {
    if(sr.empty()) return; // source rect empty, nothing to blit

    float scx = tr.w / sr.w; // scale x
    float scy = tr.h / sr.h; // scale y

    rect_t csr = sr;

    // clip the source rect if needed
    // rect_t csr = sr;
    // if(!_bounds.contains(sr)) { // source rect not entirely contained, need to clip
    //   csr = sr.intersection(_bounds);
    //   if(csr.empty()) return; // clipped source rect empty, nothing to blit

    //   // clip target rect to new clipped source rect
    //   tr = {
    //     tr.x + ((csr.x - sr.x) * scx),
    //     tr.y + ((csr.y - sr.y) * scy),
    //     csr.w * scx,
    //     csr.h * scy
    //   };
    // }

    // clip the target rect if needed
    rect_t ctr = tr;
    if(!t->_clip.contains(tr)) { // target rect not entirely contained, need to clip
      ctr = tr.intersection(t->_clip);
      if(ctr.empty()) return; // clipped source rect empty, nothing to blit

      // clip source rect to new clipped target rect
      csr = {
        csr.x + ((ctr.x - tr.x) / scx),
        csr.y + ((ctr.y - tr.y) / scy),
        ctr.w / scx,
        ctr.h / scy
      };
    }

    // render the scaled spans
    float srcstepx = csr.w / ctr.w;
    float srcstepy = csr.h / ctr.h;

    float srcx = csr.x;
    float srcy = csr.y;


    for(int y = 0; y < ctr.h; y++) {
      uint8_t *dst = (uint8_t*)t->ptr(ctr.x, ctr.y + y);
      uint8_t *src = (uint8_t*)this->ptr(0, int(srcy));
      int32_t x = int(srcx * 65536.0f);
      int32_t step = int(srcstepx * 65536.0f);

      if(this->_has_palette) {
        _span_scale_blit_rgba_rgba(dst, src, (uint8_t*)&this->_palette[0], x, step, abs(ctr.w), this->_alpha);
      }else{
        _span_scale_blit_rgba_rgba(dst, src, x, step, abs(ctr.w), this->_alpha);
      }

      srcy += srcstepy;
    }
  }


  void image_t::blit(image_t *target, rect_t tr) {
    bool invert_x = tr.w < 0.0f;
    bool invert_y = tr.h < 0.0f;

    tr.w = abs(tr.w);
    tr.h = abs(tr.h);

    int yoff = 0;
    if(tr.y < target->_clip.y) {
      yoff = target->_clip.y - tr.y;
    }

    // clip the target rect to the target bounds
    rect_t ctr = tr.intersection(target->_clip);
    if(ctr.empty()) {return;}

    // calculate the source step
    float srcstepx = (invert_x ? -1 : 1) * this->_bounds.w / tr.w;
    float srcstepy = (invert_y ? -1 : 1) * this->_bounds.h / tr.h;

    // calculate the source offset
    float srcx = invert_x ? this->_bounds.w - 1 : 0;
    float srcy = invert_y ? this->_bounds.h - 1 : 0;
    srcx += (ctr.x - tr.x) * srcstepx;
    srcy += (yoff) * srcstepy;

    int sy = ctr.y;// min(ctr.y, ctr.y + ctr.h);
    int ey = ctr.y + ctr.h;//max(ctr.y, ctr.y + ctr.h);

    for(int y = sy; y < ey; y++) {
      uint8_t *dst = (uint8_t*)target->ptr(ctr.x, y);
      uint8_t *src = (uint8_t*)this->ptr(0, int(srcy));
      int32_t x = int(srcx * 65536.0f);
      int32_t step = int(srcstepx * 65536.0f);

      if(this->_has_palette) {
        _span_scale_blit_rgba_rgba(dst, src, (uint8_t*)&this->_palette[0], x, step, abs(ctr.w), this->_alpha);
      }else{
        _span_scale_blit_rgba_rgba(dst, src, x, step, abs(ctr.w), this->_alpha);
      }

      srcy += srcstepy;
    }
  }

  void image_t::draw(shape_t *shape) {
    // pvr_reset();
    // for(auto &path : shape->paths) {
    //   pvr_add_path(path.points.data(), path.points.size(), &shape->transform);
    // }
    // pvr_render(this, _bounds, _brush);

    render(shape, this, &shape->transform, _brush);
  }

  void image_t::rectangle(rect_t r) {
    r = r.intersection(_clip);
    for(int y = r.y; y < r.y + r.h; y++) {
      this->_brush->span_func(this->_brush, r.x, y, r.w);
      //this->_brush->render_span(this, r.x, y, r.w);
    }
  }

  void image_t::span(int x, int y, int w) {
    if(y < _clip.y || y >= _clip.y + _clip.h) return;
    if(x + w < 0 || x >= _clip.x + _clip.w) return;

    if(x < 0) {
      w += x; x = 0;
    }

    if(x + w >= _clip.x + _clip.w) {
      w = _clip.x + _clip.w - x;
    }
    this->_brush->span_func(this->_brush, x, y, w);
    //this->_brush->render_span(this, x, y, w);
  }

  void image_t::circle(const point_t &p, const int &r) {

    rect_t b = rect_t(p.x - r, p.y - r, r * 2, r * 2);
    if(!b.intersects(_clip)) return;

    int ox = r, oy = 0, err = -r;
    while (ox >= oy)
    {
      int last_oy = oy;

      err += oy; oy++; err += oy;

      this->span(p.x - ox, p.y + last_oy, ox * 2 + 1);
      if (last_oy != 0) {
        this->span(p.x - ox, p.y - last_oy, ox * 2 + 1);
      }

      if(err >= 0 && ox != last_oy) {
        this->span(p.x - last_oy, p.y + ox, last_oy * 2 + 1);
        if (ox != 0) {
          this->span(p.x - last_oy, p.y - ox, last_oy * 2 + 1);
        }

        err -= ox; ox--; err -= ox;
      }
    }
  }

  int32_t orient2d(point_t p1, point_t p2, point_t p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
  }

  bool is_top_left(const point_t &p1, const point_t &p2) {
    return (p1.y == p2.y && p1.x > p2.x) || (p1.y < p2.y);
  }

  void image_t::triangle(point_t p1, point_t p2, point_t p3) {
    rect_t b(
      point_t(min(p1.x, min(p2.x, p3.x)), min(p1.y, min(p2.y, p3.y))),
      point_t(max(p1.x, max(p2.x, p3.x)), max(p1.y, max(p2.y, p3.y)))
    );

    // clip extremes to frame buffer size
    b = b.intersection(_clip);

    // if triangle completely out of bounds then don't bother!
    if (b.empty()) return;

    // fix "winding" of vertices if needed
    int32_t winding = orient2d(p1, p2, p3);
    if (winding < 0) {
      point_t t;
      t = p1; p1 = p3; p3 = t;
    }

    // bias ensures no overdraw between neighbouring triangles
    int8_t bias0 = is_top_left(p2, p3) ? 0 : -1;
    int8_t bias1 = is_top_left(p3, p1) ? 0 : -1;
    int8_t bias2 = is_top_left(p1, p2) ? 0 : -1;

    int32_t a01 = p1.y - p2.y;
    int32_t b01 = p2.x - p1.x;
    int32_t a12 = p2.y - p3.y;
    int32_t b12 = p3.x - p2.x;
    int32_t a20 = p3.y - p1.y;
    int32_t b20 = p1.x - p3.x;

    point_t tl(b.x, b.y);
    int32_t w0row = orient2d(p2, p3, tl) + bias0;
    int32_t w1row = orient2d(p3, p1, tl) + bias1;
    int32_t w2row = orient2d(p1, p2, tl) + bias2;

    pixel_func_t pf = this->_brush->pixel_func;

    for (int32_t y = 0; y < b.h; y++) {
      int32_t w0 = w0row;
      int32_t w1 = w1row;
      int32_t w2 = w2row;

      int xo = b.x;
      int yo = b.y + y;
      for (int32_t x = 0; x < b.w; x++) {
        if ((w0 | w1 | w2) >= 0) {
          pf(this->_brush, xo, yo);
        }

        xo++;
        w0 += a12; w1 += a20; w2 += a01;
      }

      w0row += b12; w1row += b20; w2row += b01;

    }
  }

  void round_rectangle(const rect_t &r, int radius) {

  }


  void ellipse(const point_t &p, const int &rx, const int &ry) {

  }


  void image_t::line(point_t p1, point_t p2) {
    rect_t b = this->_clip;
    b.w -= 1;
    b.h -= 1; // TODO: this is hacky... fix it properly
    if(!clip_line(p1, p2, b)) {
      return; // fully outside bounds, nothing to draw
    }

    int x0 = p1.x;
    int x1 = p2.x;
    int y0 = p1.y;
    int y1 = p2.y;

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    pixel_func_t pf = this->_brush->pixel_func;

    while(true) {
        pf(this->_brush, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {err += dy; x0 += sx;}
        if (e2 <= dx) {err += dx; y0 += sy;}
    }
  }

  void image_t::put(const point_t &p) {
    this->put(p.x, p.y);
  }

  void image_t::put(int x, int y) {
    x = max(int(_clip.x), min(x, int(_clip.x + _clip.w - 1)));
    y = max(int(_clip.y), min(y, int(_clip.y + _clip.h - 1)));
    this->_brush->pixel_func(this->_brush, x, y);
  }

  void image_t::put_unsafe(int x, int y) {
    this->_brush->pixel_func(this->_brush, x, y);
    //this->_brush->render_span(this, x, y, 1);
  }

  uint32_t image_t::get(const point_t &p) {
    return this->get(p.x, p.y);
  }

  uint32_t image_t::get(int x, int y) {
    x = max(int(_clip.x), min(x, int(_clip.x + _clip.w - 1)));
    y = max(int(_clip.y), min(y, int(_clip.y + _clip.h - 1)));
    return this->get_unsafe(x, y);
  }

  uint32_t image_t::get_unsafe(int x, int y) {
    if(this->_has_palette) {
      uint8_t pi = *((uint8_t *)ptr(x, y));
      return this->_palette[pi];
    }
    return *((uint32_t *)ptr(x, y));
  }



}
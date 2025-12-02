#pragma once

#include <stdint.h>
#include <string>

#include "picovector.config.hpp"
#include "rect.hpp"

using std::vector;

namespace picovector {

  typedef enum antialias_t {
    OFF   = 1,
    LOW   = 2,
    X2    = 2,
    HIGH  = 4,
    X4    = 4
  } antialias_t;

  typedef enum pixel_format_t {
    RGBA8888 = 1,
    RGBA4444 = 2,
  } pixel_format_t;

  typedef std::vector<uint32_t, PV_STD_ALLOCATOR<uint32_t>> palette_t;

  class mat3_t;
  class font_t;
  class pixel_font_t;
  class shape_t;
  class brush_t;

  class image_t {
    private:
      void           *_buffer = nullptr;
      bool            _managed_buffer = false;
      size_t          _row_stride;
      size_t          _bytes_per_pixel;

      rect_t          _bounds;
      uint8_t         _alpha = 255;
      antialias_t     _antialias = OFF;
      pixel_format_t  _pixel_format = RGBA8888;
      bool            _has_palette = false;
      brush_t        *_brush = nullptr;
      font_t         *_font = nullptr;
      pixel_font_t   *_pixel_font = nullptr;
      palette_t       _palette;

    public:
      image_t();
      image_t(image_t *source, rect_t r);
      image_t(int w, int h, pixel_format_t pixel_format=RGBA8888, bool has_palette=false);
      image_t(void *buffer, int w, int h, pixel_format_t pixel_format=RGBA8888, bool has_palette=false);
      ~image_t();

      size_t buffer_size();
      size_t bytes_per_pixel();
      bool compatible_buffer(image_t *other);
      void window(image_t *source, rect_t viewport);
      image_t window(rect_t r);
      void* ptr(int x, int y);
      uint32_t row_stride();

      rect_t bounds();

      bool has_palette();
      void delete_palette();
      void palette(uint8_t i, uint32_t c);
      uint32_t palette(uint8_t i);

      uint8_t alpha();
      void alpha(uint8_t alpha);

      antialias_t antialias();
      void antialias(antialias_t antialias);

      pixel_format_t pixel_format();
      void pixel_format(pixel_format_t pixel_format);

      brush_t *brush();
      void brush(brush_t *brush);

      font_t *font();
      void font(font_t *font);

      pixel_font_t *pixel_font();
      void pixel_font(pixel_font_t *pixel_font);

      uint32_t pixel_unsafe(int x, int y);
      uint32_t pixel(int x, int y);

      void clear();
      void rectangle(const rect_t &r);
      void circle(const point_t &p, const int &r);
      void draw(shape_t *shape);
      void blit(image_t *t, const point_t p);
      void blit(image_t *t, rect_t tr);
      void blit(image_t *t, rect_t sr, rect_t tr);



      void vspan_tex(image_t *target, point_t p, uint c, point_t uvs, point_t uve);
  };

}

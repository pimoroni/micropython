#ifndef NO_QSTR
#include <algorithm>
#include "mp_tracked_allocator.hpp"

#include "../picovector.hpp"
#include "../primitive.hpp"
#include "../shape.hpp"
#include "../image.hpp"
#include "../brush.hpp"
#include "../font.hpp"
#include "../color.hpp"
#include "../pixel_font.hpp"
#include "../blend.hpp"
#include "PNGdec.h"
#endif

using namespace picovector;

extern "C" {
  #include "types.h"

  typedef struct _brush_obj_t {
    mp_obj_base_t base;
    brush_t *brush;
  } brush_obj_t;

  typedef struct _shape_obj_t {
    mp_obj_base_t base;
    shape_t *shape;
    brush_obj_t *brush;
  } shape_obj_t;

  typedef struct _mat3_obj_t {
    mp_obj_base_t base;
    mat3_t m;
  } mat3_obj_t;

  typedef struct _png_handle_t {
    mp_obj_t fhandle;
  } png_handle_t;

  typedef struct _font_obj_t {
    mp_obj_base_t base;
    font_t font;
    uint8_t *buffer;
    uint32_t buffer_size;
  } font_obj_t;

  typedef struct _color_obj_t {
    mp_obj_base_t base;
    color_t *c;
  } color_obj_t;

  typedef struct _pixel_font_obj_t {
    mp_obj_base_t base;
    pixel_font_t *font;
    uint8_t *glyph_buffer;
    uint32_t glyph_buffer_size;
    uint8_t *glyph_data_buffer;
    uint32_t glyph_data_buffer_size;
  } pixel_font_obj_t;

  typedef struct _image_obj_t {
    mp_obj_base_t base;
    image_t *image;
    brush_obj_t *brush;
    font_obj_t *font;
    pixel_font_obj_t *pixel_font;
    void *parent;
  } image_obj_t;

  typedef struct _rect_obj_t {
    mp_obj_base_t base;
    rect_t r;
  } rect_obj_t;

  typedef struct _vec2_obj_t {
    mp_obj_base_t base;
    vec2_t v;
  } vec2_obj_t;

  // used by image.pen = N and picovector.pen() (global pen)
  extern brush_obj_t *mp_obj_to_brush(size_t n_args, const mp_obj_t *args);

  // image.cpp uses pngdec_open_callback from image_png
  extern void *pngdec_open_callback(const char *filename, int32_t *size);
  extern void pngdec_close_callback(void *handle);
  extern int32_t pngdec_read_callback(PNGFILE *png, uint8_t *p, int32_t c);
  extern int32_t pngdec_seek_callback(PNGFILE *png, int32_t p);
  extern void pngdec_decode_callback(PNGDRAW *pDraw);
}

extern rect_t mp_obj_get_rect(mp_obj_t rect_in);
extern rect_t mp_obj_get_rect_from_xywh(const mp_obj_t *args);

extern vec2_t mp_obj_get_vec2(mp_obj_t vec2_in);
extern vec2_t mp_obj_get_vec2_from_xy(const mp_obj_t *args);

extern bool mp_obj_is_rect(mp_obj_t rect_in);
extern bool mp_obj_is_vec2(mp_obj_t vec2_in);
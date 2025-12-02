#pragma once

#include <algorithm>
#include "mp_tracked_allocator.hpp"
#include "../picovector.hpp"
#include "../image.hpp"
#include "../blend.hpp"
#include "../font.hpp"
#include "../pixel_font.hpp"
#include "../brush.hpp"

#include "image_png.hpp"

#include "mp_helpers.hpp"

using namespace picovector;

extern "C" {

  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"

  extern const mp_obj_type_t type_Image;

  typedef struct _image_obj_t {
    mp_obj_base_t base;
    image_t *image;
    brush_obj_t *brush;
    font_obj_t *font;
    pixel_font_obj_t *pixel_font;
  } image_obj_t;

  mp_obj_t image__del__(mp_obj_t self_in) {
    self(self_in, image_obj_t);
    if(self->image) {
      self->image->delete_palette();
      m_del_class(image_t, self->image);
    }
    return mp_const_none;
  }

  static mp_obj_t image_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    image_obj_t *self = mp_obj_malloc_with_finaliser(image_obj_t, type);

    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);

    self->image = new(m_malloc(sizeof(image_t))) image_t(w, h);

    return MP_OBJ_FROM_PTR(self);
  }

  mp_obj_t image_load(mp_obj_t path) {
    const char *s = mp_obj_str_get_str(path);
    image_obj_t *result = mp_obj_malloc_with_finaliser(image_obj_t, &type_Image);

    PNG *png = new(PicoVector_working_buffer) PNG();
    int status = png->open(mp_obj_str_get_str(path), pngdec_open_callback, pngdec_close_callback, pngdec_read_callback, pngdec_seek_callback, pngdec_decode_callback);
    bool has_palette = png->getPixelType() == PNG_PIXEL_INDEXED;
    result->image = new(m_malloc(sizeof(image_t))) image_t(png->getWidth(), png->getHeight(), RGBA8888, has_palette);
    png->decode((void *)result->image, 0);
    png->close();
    return MP_OBJ_FROM_PTR(result);
  }


  mp_obj_t image_load_into(mp_obj_t self_in, mp_obj_t path) {
    self(self_in, image_obj_t);
    //const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    PNG *png = new(PicoVector_working_buffer) PNG();
    int status = png->open(mp_obj_str_get_str(path), pngdec_open_callback, pngdec_close_callback, pngdec_read_callback, pngdec_seek_callback, pngdec_decode_callback);
    bool has_palette = png->getPixelType() == PNG_PIXEL_INDEXED;
    png->decode((void *)self->image, 0);
    png->close();
    return mp_const_none;
  }


  mp_obj_t image_window(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    int x = mp_obj_get_int(pos_args[1]);
    int y = mp_obj_get_int(pos_args[2]);
    int w = mp_obj_get_int(pos_args[3]);
    int h = mp_obj_get_int(pos_args[4]);
    image_obj_t *result = mp_obj_malloc_with_finaliser(image_obj_t, &type_Image);
    result->image = new(m_malloc(sizeof(image_t))) image_t(self->image, rect_t(x, y, w, h));
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t image_draw(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const shape_obj_t *shape = (shape_obj_t *)MP_OBJ_TO_PTR(pos_args[1]);
    self->image->draw(shape->shape);
    return mp_const_none;
  }

  mp_obj_t image_rectangle(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    int x = mp_obj_get_int(pos_args[1]);
    int y = mp_obj_get_int(pos_args[2]);
    int w = mp_obj_get_int(pos_args[3]);
    int h = mp_obj_get_int(pos_args[4]);
    self->image->rectangle(rect_t(x, y, w, h));
    return mp_const_none;
  }


  mp_obj_t image_text(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const char *text = mp_obj_str_get_str(pos_args[1]);

    if(!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    if(self->font) {
      float x = mp_obj_get_float(pos_args[2]);
      float y = mp_obj_get_float(pos_args[3]);
      float size = mp_obj_get_float(pos_args[4]);
      self->image->font()->draw(self->image, text, x, y, size);
    }

    if(self->pixel_font) {
      int x = mp_obj_get_float(pos_args[2]);
      int y = mp_obj_get_float(pos_args[3]);
      self->image->pixel_font()->draw(self->image, text, x, y);
    }

    return mp_const_none;
  }

  mp_obj_t image_measure_text(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const char *text = mp_obj_str_get_str(pos_args[1]);

    if(!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    mp_obj_t result[2];

    if(self->font) {
      float size = mp_obj_get_float(pos_args[2]);
      rect_t r = self->image->font()->measure(self->image, text, size);
      result[0] = mp_obj_new_float(r.w);
      result[1] = mp_obj_new_float(r.h);
    }

    if(self->pixel_font) {
      rect_t r = self->image->pixel_font()->measure(self->image, text);
      result[0] = mp_obj_new_float(r.w);
      result[1] = mp_obj_new_float(r.h);
    }

    return mp_obj_new_tuple(2, result);
  }

  mp_obj_t image_vspan_tex(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const image_obj_t *src = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[1]);
    int x = mp_obj_get_float(pos_args[2]);
    int y = mp_obj_get_float(pos_args[3]);
    int c = mp_obj_get_float(pos_args[4]);
    int us = mp_obj_get_float(pos_args[5]);
    int vs = mp_obj_get_float(pos_args[6]);
    int ue = mp_obj_get_float(pos_args[7]);
    int ve = mp_obj_get_float(pos_args[8]);
    src->image->vspan_tex(self->image, point_t(x, y), c, point_t(us, vs), point_t(ue, ve));
    return mp_const_none;
  }


  mp_obj_t image_blit(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const image_obj_t *src = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[1]);
    int x = mp_obj_get_float(pos_args[2]);
    int y = mp_obj_get_float(pos_args[3]);
    src->image->blit(self->image, point_t(x, y));
    return mp_const_none;
  }

  mp_obj_t image_scale_blit(size_t n_args, const mp_obj_t *pos_args) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    const image_obj_t *src = (image_obj_t *)MP_OBJ_TO_PTR(pos_args[1]);

    if(n_args == 6) {
      int x = mp_obj_get_float(pos_args[2]);
      int y = mp_obj_get_float(pos_args[3]);
      int w = mp_obj_get_float(pos_args[4]);
      int h = mp_obj_get_float(pos_args[5]);

      src->image->blit(self->image, rect_t(x, y, w, h));
    }else{
      int sx = mp_obj_get_float(pos_args[2]);
      int sy = mp_obj_get_float(pos_args[3]);
      int sw = mp_obj_get_float(pos_args[4]);
      int sh = mp_obj_get_float(pos_args[5]);
      int dx = mp_obj_get_float(pos_args[6]);
      int dy = mp_obj_get_float(pos_args[7]);
      int dw = mp_obj_get_float(pos_args[8]);
      int dh = mp_obj_get_float(pos_args[9]);

      src->image->blit(self->image, rect_t(sx, sy, sw, sh), rect_t(dx, dy, dw, dh));
    }
    return mp_const_none;
  }

  mp_obj_t image_clear(mp_obj_t self_in) {
    self(self_in, image_obj_t);
    self->image->clear();
    return mp_const_none;
  }

  static void image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    self(self_in, image_obj_t);

    action_t action = m_attr_action(dest);

    switch(attr) {
      case MP_QSTR_width: {
        if(action == GET) {
          dest[0] = mp_obj_new_int(self->image->bounds().w);
          return;
        }
      };

      case MP_QSTR_height: {
        if(action == GET) {
          dest[0] = mp_obj_new_int(self->image->bounds().h);
          return;
        }
      };

      case MP_QSTR_has_palette: {
        if(action == GET) {
          dest[0] = mp_obj_new_bool(self->image->has_palette());
          return;
        }
      };

      case MP_QSTR_antialias: {
        if(action == GET) {
          dest[0] = mp_obj_new_int(self->image->antialias());
          return;
        }

        if(action == SET) {
          self->image->antialias((antialias_t)mp_obj_get_int(dest[1]));
          dest[0] = MP_OBJ_NULL;
          return;
        }
      };

      case MP_QSTR_alpha: {
        if(action == GET) {
          dest[0] = mp_obj_new_int(self->image->alpha());
          return;
        }

        if(action == SET) {
          self->image->alpha((int)mp_obj_get_int(dest[1]));
          dest[0] = MP_OBJ_NULL;
          return;
        }
      };

      case MP_QSTR_brush: {
        if(action == GET) {
          if(self->brush) {
            dest[0] = MP_OBJ_FROM_PTR(self->brush);
          }else{
            dest[0] = mp_const_none;
          }
          return;
        }

        if(action == SET) {
          if(!mp_obj_is_type(dest[1], &type_Brush)) {
            mp_raise_TypeError(MP_ERROR_TEXT("value must be of type Brush"));
          }
          self->brush = (brush_obj_t *)dest[1];
          self->image->brush(self->brush->brush);
          dest[0] = MP_OBJ_NULL;
          return;
        }
      };

      case MP_QSTR_font: {
        if(action == GET) {
          if(self->font || self->pixel_font) {
            if(self->font) {
              dest[0] = MP_OBJ_FROM_PTR(self->font);
            }else{
              dest[0] = MP_OBJ_FROM_PTR(self->pixel_font);
            }
          }else{
            dest[0] = mp_const_none;
          }
          return;
        }

        if(action == SET) {
          if(!mp_obj_is_type(dest[1], &type_Font) && !mp_obj_is_type(dest[1], &type_PixelFont)) {
            mp_raise_TypeError(MP_ERROR_TEXT("value must be of type Font or PixelFont"));
          }
          if(mp_obj_is_type(dest[1], &type_Font)) {
            self->font = (font_obj_t *)dest[1];
            self->pixel_font = nullptr;
            self->image->font(&self->font->font);
          }
          if(mp_obj_is_type(dest[1], &type_PixelFont)) {
            self->pixel_font = (pixel_font_obj_t *)dest[1];
            self->font = nullptr;
            self->image->pixel_font(self->pixel_font->font);
          }
          dest[0] = MP_OBJ_NULL;
          return;
        }
      };
    }

    // we didn't handle this, fall back to alternative methods
    dest[1] = MP_OBJ_SENTINEL;
  }

  mp_int_t image_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->image->ptr(0, 0);
    bufinfo->len = self->image->buffer_size();
    bufinfo->typecode = 'B';
    return 0;
}

  static MP_DEFINE_CONST_FUN_OBJ_1(image__del___obj, image__del__);

  static MP_DEFINE_CONST_FUN_OBJ_1(image_load_obj, image_load);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(image_load_static_obj, MP_ROM_PTR(&image_load_obj));

  static MP_DEFINE_CONST_FUN_OBJ_2(image_load_into_obj, image_load_into);

  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_window_obj, 5, image_window);

  static MP_DEFINE_CONST_FUN_OBJ_1(image_clear_obj, image_clear);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_rectangle_obj, 5, image_rectangle);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_draw_obj, 2, image_draw);

  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_vspan_tex_obj, 4, image_vspan_tex);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_blit_obj, 4, image_blit);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_scale_blit_obj, 4, image_scale_blit);

  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_text_obj, 3, image_text);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(image_measure_text_obj, 2, image_measure_text);

  static const mp_rom_map_elem_t image_locals_dict_table[] = {
      { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&image__del___obj) },
      { MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&image_draw_obj) },
      { MP_ROM_QSTR(MP_QSTR_window), MP_ROM_PTR(&image_window_obj) },
      { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&image_clear_obj) },
      { MP_ROM_QSTR(MP_QSTR_rectangle), MP_ROM_PTR(&image_rectangle_obj) },
      { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&image_text_obj) },
      { MP_ROM_QSTR(MP_QSTR_measure_text), MP_ROM_PTR(&image_measure_text_obj) },
      { MP_ROM_QSTR(MP_QSTR_vspan_tex), MP_ROM_PTR(&image_vspan_tex_obj) },
      { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&image_blit_obj) },
      { MP_ROM_QSTR(MP_QSTR_scale_blit), MP_ROM_PTR(&image_scale_blit_obj) },
      { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&image_load_static_obj) },
      { MP_ROM_QSTR(MP_QSTR_load_into), MP_ROM_PTR(&image_load_into_obj) },
      { MP_ROM_QSTR(MP_QSTR_X4), MP_ROM_INT(antialias_t::X4)},
      { MP_ROM_QSTR(MP_QSTR_X2), MP_ROM_INT(antialias_t::X2)},
      { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(antialias_t::OFF)},
  };
  static MP_DEFINE_CONST_DICT(image_locals_dict, image_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
      type_Image,
      MP_QSTR_Image,
      MP_TYPE_FLAG_NONE,
      make_new, (const void *)image_new,
      attr, (const void *)image_attr,
      buffer, (const void *)image_get_buffer,
      locals_dict, &image_locals_dict
  );

}

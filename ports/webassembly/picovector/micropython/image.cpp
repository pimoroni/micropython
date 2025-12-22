#include "mp_helpers.hpp"
#include "picovector.hpp"

extern "C" {

  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"

  mp_obj_t image__del__(mp_obj_t self_in) {
    self(self_in, image_obj_t);
    if(self->image) {
      //self->image->delete_palette();
      m_del_class(image_t, self->image);
    }
    return mp_const_none;
  }
  static MP_DEFINE_CONST_FUN_OBJ_1(image__del___obj, image__del__);

MPY_BIND_NEW(image, {
    image_obj_t *self = mp_obj_malloc_with_finaliser(image_obj_t, type);

    int w = mp_obj_get_int(args[0]);
    int h = mp_obj_get_int(args[1]);

    if (n_args > 2) {
      mp_buffer_info_t bufinfo;
      mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
      self->image = new(m_malloc(sizeof(image_t))) image_t(bufinfo.buf, w, h);
    } else {
      self->image = new(m_malloc(sizeof(image_t))) image_t(w, h);
    }

    return MP_OBJ_FROM_PTR(self);
})

MPY_BIND_STATICMETHOD_ARGS1(load, path, {
    const char *s = mp_obj_str_get_str(path);
    image_obj_t *result = mp_obj_malloc_with_finaliser(image_obj_t, &type_image);

    PNG *png = new(PicoVector_working_buffer) PNG();
    int status = png->open(mp_obj_str_get_str(path), pngdec_open_callback, pngdec_close_callback, pngdec_read_callback, pngdec_seek_callback, pngdec_decode_callback);
    bool has_palette = png->getPixelType() == PNG_PIXEL_INDEXED;
    result->image = new(m_malloc(sizeof(image_t))) image_t(png->getWidth(), png->getHeight(), RGBA8888, has_palette);
    png->decode((void *)result->image, 0);
    png->close();
    return MP_OBJ_FROM_PTR(result);
  })

MPY_BIND_CLASSMETHOD_ARGS1(load_into, path, {
    self(self_in, image_obj_t);
    //const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    PNG *png = new(PicoVector_working_buffer) PNG();
    int status = png->open(mp_obj_str_get_str(path), pngdec_open_callback, pngdec_close_callback, pngdec_read_callback, pngdec_seek_callback, pngdec_decode_callback);
    bool has_palette = png->getPixelType() == PNG_PIXEL_INDEXED;
    png->decode((void *)self->image, 0);
    png->close();
    return mp_const_none;
  })


MPY_BIND_VAR(2, window, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    int x;
    int y;
    int w;
    int h;

    if (mp_obj_is_type(args[1], &type_rect)) {
      const rect_obj_t *rect = (rect_obj_t *)MP_OBJ_TO_PTR(args[1]);
      x = rect->rect.x;
      y = rect->rect.y;
      w = rect->rect.w;
      h = rect->rect.h;
    }else{
      x = mp_obj_get_float(args[1]);
      y = mp_obj_get_float(args[2]);
      w = mp_obj_get_float(args[3]);
      h = mp_obj_get_float(args[4]);
    }

    image_obj_t *result = mp_obj_malloc_with_finaliser(image_obj_t, &type_image);
    result->image = new(m_malloc(sizeof(image_t))) image_t(self->image, rect_t(x, y, w, h));
    result->parent = (void*)self;
    return MP_OBJ_FROM_PTR(result);
  })


  // MPY_BIND_VAR(2, clip, {
  //   const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

  //   if(mp_obj_is_type(args[1], &type_rect)) {
  //     const rect_obj_t *rect = (rect_obj_t *)MP_OBJ_TO_PTR(args[1]);
  //     self->image->clip(rect->rect);
  //     return mp_const_none;
  //   }

  //   if(n_args == 5) {
  //     int x = mp_obj_get_float(args[1]);
  //     int y = mp_obj_get_float(args[2]);
  //     int w = mp_obj_get_float(args[3]);
  //     int h = mp_obj_get_float(args[4]);
  //     self->image->clip(rect_t(x, y, w, h));
  //     return mp_const_none;
  //   }

  //   mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either clip(r) or clip(x, y, w, h)"));
  // })

  MPY_BIND_VAR(2, shape, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if (mp_obj_is_type(args[1], &type_shape)) {
      const shape_obj_t *shape = (shape_obj_t *)MP_OBJ_TO_PTR(args[1]);
      self->image->draw(shape->shape);
      return mp_const_none;
    }

    if (mp_obj_is_type(args[1], &mp_type_list)) {
      size_t len;
      mp_obj_t *items;
      mp_obj_list_get(args[1], &len, &items);
      for(size_t i = 0; i < len; i++) {
        const shape_obj_t *shape = (shape_obj_t *)MP_OBJ_TO_PTR(items[i]);
        self->image->draw(shape->shape);
      }
      return mp_const_none;
    }

    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either shape(s) or shape([s1, s2, s3, ...])"));
  })


  MPY_BIND_VAR(2, rectangle, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if(mp_obj_is_rect(args[1])) {
      self->image->rectangle(mp_obj_get_rect(args[1]));
      return mp_const_none;
    }

    if(n_args == 5) {
      self->image->rectangle(mp_obj_get_rect_from_xywh(&args[1]));
      return mp_const_none;
    }

    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either rectangle(r) or rectangle(x, y, w, h)"));
  })

  MPY_BIND_VAR(3, line, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if(mp_obj_is_type(args[1], &type_point) && mp_obj_is_type(args[2], &type_point)) {
      point_obj_t *p1 = (point_obj_t *)MP_OBJ_TO_PTR(args[1]);
      point_obj_t *p2 = (point_obj_t *)MP_OBJ_TO_PTR(args[2]);
      self->image->line(p1->point, p2->point);
      return mp_const_none;
    }

    if(n_args == 5) {
      int x1 = mp_obj_get_float(args[1]);
      int y1 = mp_obj_get_float(args[2]);
      int x2 = mp_obj_get_float(args[3]);
      int y2 = mp_obj_get_float(args[4]);
      self->image->line(point_t(x1, y1), point_t(x2, y2));
      return mp_const_none;
    }

    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either line(p1, p2) or line(x1, y1, x2, y2)"));
  })


  MPY_BIND_VAR(3, circle, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if(mp_obj_is_type(args[1], &type_point)) {
      point_obj_t *p = (point_obj_t *)MP_OBJ_TO_PTR(args[1]);
      float r = mp_obj_get_float(args[2]);
      self->image->circle(p->point, r);
      return mp_const_none;
    }

    if(n_args == 4) {
      int x = mp_obj_get_float(args[1]);
      int y = mp_obj_get_float(args[2]);
      int r = mp_obj_get_float(args[3]);
      self->image->circle(point_t(x, y), r);
      return mp_const_none;
    }

    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either circle(p, r) or circle(x, y, r)"));
  })

  MPY_BIND_VAR(4, triangle, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if(mp_obj_is_type(args[1], &type_point) && mp_obj_is_type(args[2], &type_point) && mp_obj_is_type(args[3], &type_point)) {
      point_obj_t *p1 = (point_obj_t *)MP_OBJ_TO_PTR(args[1]);
      point_obj_t *p2 = (point_obj_t *)MP_OBJ_TO_PTR(args[2]);
      point_obj_t *p3 = (point_obj_t *)MP_OBJ_TO_PTR(args[3]);
      self->image->triangle(p1->point, p2->point, p3->point);
      return mp_const_none;
    }

    if(n_args == 7) {
      int x1 = mp_obj_get_float(args[1]);
      int y1 = mp_obj_get_float(args[2]);
      int x2 = mp_obj_get_float(args[3]);
      int y2 = mp_obj_get_float(args[4]);
      int x3 = mp_obj_get_float(args[5]);
      int y3 = mp_obj_get_float(args[6]);
      self->image->triangle(point_t(x1, y1), point_t(x2, y2), point_t(x3, y3));
      return mp_const_none;
    }

    mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected either triangle(p1, p2, p3) or triangle(x1, y1, x2, y2, x3, y3)"));
  })


MPY_BIND_VAR(3, get, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_float(args[1]);
    int y = mp_obj_get_float(args[2]);
    color_obj_t *color = mp_obj_malloc(color_obj_t, &type_color);
    color->c = self->image->get(x, y);
    return MP_OBJ_FROM_PTR(color);
  })

MPY_BIND_VAR(3, put, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_float(args[1]);
    int y = mp_obj_get_float(args[2]);
    self->image->put(x, y);
    return mp_const_none;
  })

MPY_BIND_VAR(3, text, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    const char *text = mp_obj_str_get_str(args[1]);

    if(!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    if(self->font) {
      float x = mp_obj_get_float(args[2]);
      float y = mp_obj_get_float(args[3]);
      float size = mp_obj_get_float(args[4]);
      self->image->font()->draw(self->image, text, x, y, size);
    }

    if(self->pixel_font) {
      int x = mp_obj_get_float(args[2]);
      int y = mp_obj_get_float(args[3]);
      self->image->pixel_font()->draw(self->image, text, x, y);
    }

    return mp_const_none;
  })

MPY_BIND_VAR(2, measure_text, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    const char *text = mp_obj_str_get_str(args[1]);

    if(!self->font && !self->pixel_font) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("target image has no font"));
    }

    mp_obj_t result[2];

    if(self->font) {
      float size = mp_obj_get_float(args[2]);
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
  })

MPY_BIND_VAR(9, vspan_tex, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);
    const image_obj_t *src = (image_obj_t *)MP_OBJ_TO_PTR(args[1]);
    int x = mp_obj_get_float(args[2]);
    int y = mp_obj_get_float(args[3]);
    int c = mp_obj_get_float(args[4]);
    int us = mp_obj_get_float(args[5]);
    int vs = mp_obj_get_float(args[6]);
    int ue = mp_obj_get_float(args[7]);
    int ve = mp_obj_get_float(args[8]);
    src->image->vspan_tex(self->image, point_t(x, y), c, point_t(us, vs), point_t(ue, ve));
    return mp_const_none;
  })

MPY_BIND_VAR(3, blit, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    if(mp_obj_is_type(args[1], &type_image)) {

      const image_obj_t *src = (image_obj_t *)MP_OBJ_TO_PTR(args[1]);

      if(n_args == 3 && mp_obj_is_point(args[2])) {
        src->image->blit(self->image, mp_obj_get_point(args[2]));
        return mp_const_none;
      }

      if(n_args == 3 && mp_obj_is_rect(args[2])) {
        src->image->blit(self->image, mp_obj_get_rect(args[2]));
        return mp_const_none;
      }

      if(n_args == 4 && mp_obj_is_rect(args[2]) && mp_obj_is_rect(args[3])) {
        src->image->blit(self->image, mp_obj_get_rect(args[2]), mp_obj_get_rect(args[3]));
        return mp_const_none;
      }

    }

    mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("invalid parameter, expected blit(image, point), blit(image, rect) or blit(image, source_rect, dest_rect)"));
  })

MPY_BIND_VAR(1, clear, {
    const image_obj_t *self = (image_obj_t *)MP_OBJ_TO_PTR(args[0]);

    // if(n_args == 2) {
    //   const color_obj_t *color = (color_obj_t *)MP_OBJ_TO_PTR(args[1]);
    //   self->image->clear(color->c);
    // }else{
      self->image->clear();
//    }

    return mp_const_none;
  })

MPY_BIND_ATTR(image, {
    self(self_in, image_obj_t);

    action_t action = m_attr_action(dest);

    switch(attr) {
      case MP_QSTR_raw: {
        if(action == GET) {
          mp_obj_t raw = mp_obj_new_bytearray_by_ref(self->image->buffer_size(), self->image->ptr(0, 0));
          dest[0] = raw;
          return;
        }
      };

      case MP_QSTR_clip: {
        if(action == GET) {
          rect_obj_t *result = mp_obj_malloc(rect_obj_t, &type_rect);
          result->rect = self->image->clip();
          dest[0] = MP_OBJ_FROM_PTR(result);
          return;
        }

        if(action == SET) {
          if(!mp_obj_is_type(dest[1], &type_rect)) {
            mp_raise_TypeError(MP_ERROR_TEXT("value must be of type rect"));
          }

          rect_obj_t * r = (rect_obj_t *)dest[1];
          self->image->clip(r->rect);
          dest[0] = MP_OBJ_NULL;
          return;
        }
      };

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

      case MP_QSTR_pen: {
        if(action == GET) {
          if(self->brush) {
            dest[0] = MP_OBJ_FROM_PTR(self->brush);
          }else{
            dest[0] = mp_const_none;
          }
          return;
        }

        if(action == SET) {
          brush_obj_t *brush = mp_obj_to_brush(self->image, 1, &dest[1]);
          if(!brush){
            mp_raise_TypeError(MP_ERROR_TEXT("value must be of type brush or color"));
          }
          self->brush = brush;
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
          if(!mp_obj_is_type(dest[1], &type_font) && !mp_obj_is_type(dest[1], &type_pixel_font)) {
            mp_raise_TypeError(MP_ERROR_TEXT("value must be of type Font or PixelFont"));
          }
          if(mp_obj_is_type(dest[1], &type_font)) {
            self->font = (font_obj_t *)dest[1];
            self->pixel_font = nullptr;
            self->image->font(&self->font->font);
          }
          if(mp_obj_is_type(dest[1], &type_pixel_font)) {
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
  })

MPY_BIND_LOCALS_DICT(image,
      { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&image__del___obj) },

      MPY_BIND_ROM_PTR_STATIC(load),
      MPY_BIND_ROM_PTR(load_into),
      MPY_BIND_ROM_PTR(window),
      //MPY_BIND_ROM_PTR(clip),

      // primitives
      MPY_BIND_ROM_PTR(clear),
      MPY_BIND_ROM_PTR(rectangle),
      MPY_BIND_ROM_PTR(line),
      MPY_BIND_ROM_PTR(circle),
      MPY_BIND_ROM_PTR(triangle),
      MPY_BIND_ROM_PTR(get),
      MPY_BIND_ROM_PTR(put),

      // vector
      MPY_BIND_ROM_PTR(shape),

      // text
      MPY_BIND_ROM_PTR(text),
      MPY_BIND_ROM_PTR(measure_text),

      // blitting
      MPY_BIND_ROM_PTR(vspan_tex),
      MPY_BIND_ROM_PTR(blit),

      // TODO: Just define these in MicroPython?
      { MP_ROM_QSTR(MP_QSTR_X4), MP_ROM_INT(antialias_t::X4)},
      { MP_ROM_QSTR(MP_QSTR_X2), MP_ROM_INT(antialias_t::X2)},
      { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(antialias_t::OFF)},
)

  MP_DEFINE_CONST_OBJ_TYPE(
      type_image,
      MP_QSTR_image,
      MP_TYPE_FLAG_NONE,
      make_new, (const void *)image_new,
      attr, (const void *)image_attr,
      locals_dict, &image_locals_dict
  );

}

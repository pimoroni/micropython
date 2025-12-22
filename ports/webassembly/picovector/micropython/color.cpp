#include "mp_tracked_allocator.hpp"

#include "mp_helpers.hpp"
#include "picovector.hpp"

extern "C" {

  #include "py/runtime.h"

  MPY_BIND_STATICMETHOD_VAR(3, rgb, {
    int r = (int)mp_obj_get_float(args[0]);
    int g = (int)mp_obj_get_float(args[1]);
    int b = (int)mp_obj_get_float(args[2]);
    int a = n_args > 3 ? (int)mp_obj_get_float(args[3]) : 255;
    color_obj_t *color = mp_obj_malloc(color_obj_t, &type_color);
    color->c = rgba(r, g, b, a);
    return MP_OBJ_FROM_PTR(color);
  })

  MPY_BIND_STATICMETHOD_VAR(3, hsv, {
    int h = (int)mp_obj_get_float(args[0]);
    int s = (int)mp_obj_get_float(args[1]);
    int v = (int)mp_obj_get_float(args[2]);
    int a = n_args > 3 ? (int)mp_obj_get_float(args[3]) : 255;
    color_obj_t *color = mp_obj_malloc(color_obj_t, &type_color);
    color->c = hsv(h, s, v, a);
    return MP_OBJ_FROM_PTR(color);
  })

  MPY_BIND_STATICMETHOD_VAR(3, oklch, {
    int l = (int)mp_obj_get_float(args[0]);
    int c = (int)mp_obj_get_float(args[1]);
    int h = (int)mp_obj_get_float(args[2]);
    int a = n_args > 3 ? (int)mp_obj_get_float(args[3]) : 255;
    color_obj_t *color = mp_obj_malloc(color_obj_t, &type_color);
    color->c = oklch(l, c, h, a);
    return MP_OBJ_FROM_PTR(color);
  })

  MPY_BIND_VAR(2, blend, {
    const color_obj_t *self = (color_obj_t *)MP_OBJ_TO_PTR(args[0]);
    const color_obj_t *other = (color_obj_t *)MP_OBJ_TO_PTR(args[1]);
    uint8_t *src = (uint8_t*)&other->c;
    color_obj_t *result = mp_obj_malloc(color_obj_t, &type_color);
    result->c = self->c;
    blend_rgba_rgba((uint8_t*)&result->c, src[0], src[1], src[2], src[3]);
    return MP_OBJ_FROM_PTR(result);
  })

  static inline uint8_t darken_u8(uint8_t c, uint8_t factor) {
    return (uint8_t)((c * factor) >> 8);
  }

  MPY_BIND_VAR(2, darken, {
    const color_obj_t *self = (color_obj_t *)MP_OBJ_TO_PTR(args[0]);
    int v = 255 - (int)mp_obj_get_float(args[1]);
    color_obj_t *result = mp_obj_malloc(color_obj_t, &type_color);
    result->c = self->c;
    set_r(&result->c, darken_u8(get_r(&self->c), v));
    set_g(&result->c, darken_u8(get_g(&self->c), v));
    set_b(&result->c, darken_u8(get_b(&self->c), v));
    return MP_OBJ_FROM_PTR(result);
  })

  static inline uint8_t lighten_u8(uint8_t c, uint factor) {
    // factor >= 256 (1.0x)
    uint16_t v = (c * factor) >> 8;
    return v > 255 ? 255 : (uint8_t)v;
  }

  MPY_BIND_VAR(2, lighten, {
    const color_obj_t *self = (color_obj_t *)MP_OBJ_TO_PTR(args[0]);
    int v = 256 + (int)mp_obj_get_float(args[1]);
    color_obj_t *result = mp_obj_malloc(color_obj_t, &type_color);
    result->c = self->c;
    set_r(&result->c, lighten_u8(get_r(&self->c), v));
    set_g(&result->c, lighten_u8(get_g(&self->c), v));
    set_b(&result->c, lighten_u8(get_b(&self->c), v));
    return MP_OBJ_FROM_PTR(result);
  })

  static void attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    self(self_in, color_obj_t);

    action_t action = m_attr_action(dest);

    constexpr size_t GET = 0b1 << 31;
    constexpr size_t SET = 0b1 << 30;
    constexpr size_t DELETE = 0b1 << 29;

    switch(attr | action) {
      case MP_QSTR_r | GET:
        dest[0] = mp_obj_new_int(get_r(&self->c));
        return;

      case MP_QSTR_r | SET:
        set_r(&self->c, (int)mp_obj_get_float(dest[1]));
        dest[0] = MP_OBJ_NULL;
        return;

      case MP_QSTR_g | GET:
        dest[0] = mp_obj_new_int(get_g(&self->c));
        return;

      case MP_QSTR_g | SET:
        set_g(&self->c, (int)mp_obj_get_float(dest[1]));
        dest[0] = MP_OBJ_NULL;
        return;

      case MP_QSTR_b | GET:
        dest[0] = mp_obj_new_int(get_b(&self->c));
        return;

      case MP_QSTR_b | SET:
        set_b(&self->c, (int)mp_obj_get_float(dest[1]));
        dest[0] = MP_OBJ_NULL;
        return;

      case MP_QSTR_a | GET:
        dest[0] = mp_obj_new_int(get_a(&self->c));
        return;

      case MP_QSTR_a | SET:
        set_a(&self->c, (int)mp_obj_get_float(dest[1]));
        dest[0] = MP_OBJ_NULL;
        return;

      case MP_QSTR_raw | GET:
        dest[0] = mp_obj_new_int(self->c);
        return;

      case MP_QSTR_raw | SET:
        self->c = mp_obj_get_int(dest[1]);
        dest[0] = MP_OBJ_NULL;
        return;
    };

    dest[1] = MP_OBJ_SENTINEL;
  }

  // default palette based on Dawnbringer 16
  const color_obj_t color_black_obj  = {.base = {.type = &type_color}, .c = 0xff281e14u};
  const color_obj_t color_grape_obj  = {.base = {.type = &type_color}, .c = 0xff342444u};
  const color_obj_t color_navy_obj   = {.base = {.type = &type_color}, .c = 0xff6d3430u};
  const color_obj_t color_grey_obj   = {.base = {.type = &type_color}, .c = 0xff4e4a4eu};
  const color_obj_t color_brown_obj  = {.base = {.type = &type_color}, .c = 0xff304c85u};
  const color_obj_t color_green_obj  = {.base = {.type = &type_color}, .c = 0xff246534u};
  const color_obj_t color_red_obj    = {.base = {.type = &type_color}, .c = 0xff4846d0u};
  const color_obj_t color_taupe_obj  = {.base = {.type = &type_color}, .c = 0xff617175u};
  const color_obj_t color_blue_obj   = {.base = {.type = &type_color}, .c = 0xffce7d59u};
  const color_obj_t color_orange_obj = {.base = {.type = &type_color}, .c = 0xff2c7dd2u};
  const color_obj_t color_smoke_obj  = {.base = {.type = &type_color}, .c = 0xffa19585u};
  const color_obj_t color_lime_obj   = {.base = {.type = &type_color}, .c = 0xff2caa6du};
  const color_obj_t color_latte_obj  = {.base = {.type = &type_color}, .c = 0xff99aad2u};
  const color_obj_t color_cyan_obj   = {.base = {.type = &type_color}, .c = 0xffcac26du};
  const color_obj_t color_yellow_obj = {.base = {.type = &type_color}, .c = 0xff5ed4dau};
  const color_obj_t color_white_obj  = {.base = {.type = &type_color}, .c = 0xffd6eedeu};

  MPY_BIND_LOCALS_DICT(color,
    // static color generators
    MPY_BIND_ROM_PTR_STATIC(rgb),
    MPY_BIND_ROM_PTR_STATIC(hsv),
    MPY_BIND_ROM_PTR_STATIC(oklch),

    // color modifiers
    MPY_BIND_ROM_PTR(darken),
    MPY_BIND_ROM_PTR(lighten),
    MPY_BIND_ROM_PTR(blend),

    { MP_ROM_QSTR(MP_QSTR_black),  MP_ROM_PTR(&color_black_obj) },
    { MP_ROM_QSTR(MP_QSTR_grape),  MP_ROM_PTR(&color_grape_obj) },
    { MP_ROM_QSTR(MP_QSTR_navy),   MP_ROM_PTR(&color_navy_obj) },
    { MP_ROM_QSTR(MP_QSTR_grey),   MP_ROM_PTR(&color_grey_obj) },
    { MP_ROM_QSTR(MP_QSTR_brown),  MP_ROM_PTR(&color_brown_obj) },
    { MP_ROM_QSTR(MP_QSTR_green),  MP_ROM_PTR(&color_green_obj) },
    { MP_ROM_QSTR(MP_QSTR_red),    MP_ROM_PTR(&color_red_obj) },
    { MP_ROM_QSTR(MP_QSTR_taupe),  MP_ROM_PTR(&color_taupe_obj) },
    { MP_ROM_QSTR(MP_QSTR_blue),   MP_ROM_PTR(&color_blue_obj) },
    { MP_ROM_QSTR(MP_QSTR_orange), MP_ROM_PTR(&color_orange_obj) },
    { MP_ROM_QSTR(MP_QSTR_smoke),  MP_ROM_PTR(&color_smoke_obj) },
    { MP_ROM_QSTR(MP_QSTR_lime),   MP_ROM_PTR(&color_lime_obj) },
    { MP_ROM_QSTR(MP_QSTR_latte),  MP_ROM_PTR(&color_latte_obj) },
    { MP_ROM_QSTR(MP_QSTR_cyan),   MP_ROM_PTR(&color_cyan_obj) },
    { MP_ROM_QSTR(MP_QSTR_yellow), MP_ROM_PTR(&color_yellow_obj) },
    { MP_ROM_QSTR(MP_QSTR_white),  MP_ROM_PTR(&color_white_obj) },
  )

  MP_DEFINE_CONST_OBJ_TYPE(
      type_color,
      MP_QSTR_color,
      MP_TYPE_FLAG_NONE,
      attr, (const void *)attr,
      locals_dict, &color_locals_dict
  );
}
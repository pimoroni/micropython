#include "picovector.hpp"
#include "mp_helpers.hpp"

action_t m_attr_action(mp_obj_t *dest) {
  if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL) {return GET;}
  if(dest[0] == MP_OBJ_NULL && dest[1] != MP_OBJ_NULL) {return DELETE;}
  return SET;
}

uint32_t ru32(mp_obj_t file) {
  int error;
  uint32_t result;
  mp_stream_read_exactly(file, &result, 4, &error);
  return __builtin_bswap32(result);
}

uint16_t ru16(mp_obj_t file) {
  int error;
  uint16_t result;
  mp_stream_read_exactly(file, &result, 2, &error);
  return __builtin_bswap16(result);
}

uint8_t ru8(mp_obj_t file) {
  int error;
  uint8_t result;
  mp_stream_read_exactly(file, &result, 1, &error);
  return result;
}

int8_t rs8(mp_obj_t file) {
  int error;
  int8_t result;
  mp_stream_read_exactly(file, &result, 1, &error);
  return result;
}

extern "C" {
  #include "py/runtime.h"

  mp_obj_t modpicovector___init__(void) {
      return mp_const_none;
  }

  brush_obj_t *mp_obj_to_brush(size_t n_args, const mp_obj_t *args) {
    if(n_args == 1 && mp_obj_is_type(args[0], &type_brush)) {
      return (brush_obj_t *)MP_OBJ_TO_PTR(args[0]);
    }

    if(n_args == 1 && mp_obj_is_type(args[0], &type_color)) {
      color_obj_t *color = (color_obj_t *)MP_OBJ_TO_PTR(args[0]);
      brush_obj_t *brush = mp_obj_malloc(brush_obj_t, &type_brush);
      brush->brush = m_new_class(color_brush_t, *color->c);
      return brush;
    }

    // if(n_args == 1 && mp_obj_is_int(args[0])) {
    //   brush_obj_t *brush = mp_obj_malloc(brush_obj_t, &type_brush);
    //   brush->brush = m_new_class(color_brush_t, mp_obj_get_uint(args[0]));
    //   return brush;
    // }

    /*
    if(n_args >= 3 && mp_obj_is_int(args[0]) && mp_obj_is_int(args[1]) && mp_obj_is_int(args[2])) {
      brush_obj_t *brush = mp_obj_malloc(brush_obj_t, &type_brush);
      int r = mp_obj_get_int(args[0]);
      int g = mp_obj_get_int(args[1]);
      int b = mp_obj_get_int(args[2]);
      int a = (n_args > 3 && mp_obj_is_int(args[3])) ? mp_obj_get_int(args[3]) : 255;
      brush->brush = m_new_class(color_brush_t, target, rgba(r, g, b, a));
      return brush;
    }
    */

    return nullptr;
  }

}

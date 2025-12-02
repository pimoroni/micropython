#pragma once

#include "mp_tracked_allocator.hpp"
#include "../matrix.hpp"

#include "mp_helpers.hpp"

using namespace picovector;

extern "C" {

  #include "py/runtime.h"

  extern const mp_obj_type_t type_Matrix;

  typedef struct _matrix_obj_t {
    mp_obj_base_t base;
    mat3_t m;
  } matrix_obj_t;


  static mp_obj_t matrix_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    matrix_obj_t *self = mp_obj_malloc(matrix_obj_t, type);
    self->m = mat3_t();
    return MP_OBJ_FROM_PTR(self);
  }


  mp_obj_t matrix_rotate(size_t n_args, const mp_obj_t *pos_args) {
    matrix_obj_t *self = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    float a = mp_obj_get_float(pos_args[1]);
    matrix_obj_t *result = mp_obj_malloc(matrix_obj_t, &type_Matrix);
    result->m = self->m.rotate(a);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t matrix_rotate_radians(size_t n_args, const mp_obj_t *pos_args) {
    matrix_obj_t *self = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    float a = mp_obj_get_float(pos_args[1]);
    matrix_obj_t *result = mp_obj_malloc(matrix_obj_t, &type_Matrix);
    result->m = self->m.rotate_radians(a);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t matrix_translate(size_t n_args, const mp_obj_t *pos_args) {
    matrix_obj_t *self = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    float x = mp_obj_get_float(pos_args[1]);
    float y = mp_obj_get_float(pos_args[2]);
    matrix_obj_t *result = mp_obj_malloc(matrix_obj_t, &type_Matrix);
    result->m = self->m.translate(x, y);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t matrix_scale(size_t n_args, const mp_obj_t *pos_args) {
    matrix_obj_t *self = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    float x = mp_obj_get_float(pos_args[1]);
    float y = x;
    if(n_args > 2) {
      y = mp_obj_get_float(pos_args[2]);
    }
    matrix_obj_t *result = mp_obj_malloc(matrix_obj_t, &type_Matrix);
    result->m = self->m.scale(x, y);
    return MP_OBJ_FROM_PTR(result);
  }

  mp_obj_t matrix_multiply(size_t n_args, const mp_obj_t *pos_args) {
    matrix_obj_t *self = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    matrix_obj_t *other = (matrix_obj_t *)MP_OBJ_TO_PTR(pos_args[1]);
    matrix_obj_t *result = mp_obj_malloc(matrix_obj_t, &type_Matrix);
    result->m = self->m.multiply(other->m);
    return MP_OBJ_FROM_PTR(result);
  }

  static MP_DEFINE_CONST_FUN_OBJ_VAR(matrix_rotate_obj, 2, matrix_rotate);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(matrix_rotate_radians_obj, 2, matrix_rotate_radians);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(matrix_translate_obj, 3, matrix_translate);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(matrix_scale_obj, 2, matrix_scale);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(matrix_multiply_obj, 2, matrix_multiply);

  static const mp_rom_map_elem_t matrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rotate), MP_ROM_PTR(&matrix_rotate_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotate_radians), MP_ROM_PTR(&matrix_rotate_radians_obj) },
    { MP_ROM_QSTR(MP_QSTR_translate), MP_ROM_PTR(&matrix_translate_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&matrix_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_multiply), MP_ROM_PTR(&matrix_multiply_obj) },
  };
  static MP_DEFINE_CONST_DICT(matrix_locals_dict, matrix_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
      type_Matrix,
      MP_QSTR_Matrix,
      MP_TYPE_FLAG_NONE,
      make_new, (const void *)matrix_new,
      locals_dict, &matrix_locals_dict
  );

}




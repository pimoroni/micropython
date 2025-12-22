#include "mp_helpers.hpp"
#include "picovector.hpp"

extern "C" {

  #include "py/runtime.h"


  MPY_BIND_NEW(matrix, {
    mat3_obj_t *self = mp_obj_malloc(mat3_obj_t, type);
    self->m = mat3_t();
    return MP_OBJ_FROM_PTR(self);
  })


  MPY_BIND_VAR(2, rotate, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    float a = mp_obj_get_float(args[1]);
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.rotate(a);
    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_VAR(2, rotate_radians, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    float a = mp_obj_get_float(args[1]);
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.rotate_radians(a);
    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_VAR(3, translate, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    float x = mp_obj_get_float(args[1]);
    float y = mp_obj_get_float(args[2]);
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.translate(x, y);
    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_VAR(2, scale, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    float x = mp_obj_get_float(args[1]);
    float y = x;
    if(n_args > 2) {
      y = mp_obj_get_float(args[2]);
    }
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.scale(x, y);
    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_VAR(2, multiply, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    mat3_obj_t *other = (mat3_obj_t *)MP_OBJ_TO_PTR(args[1]);
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.multiply(other->m);
    return MP_OBJ_FROM_PTR(result);
  })

  MPY_BIND_VAR(2, inverse, {
    mat3_obj_t *self = (mat3_obj_t *)MP_OBJ_TO_PTR(args[0]);
    mat3_obj_t *result = mp_obj_malloc(mat3_obj_t, &type_mat3);
    result->m = self->m.inverse();
    return MP_OBJ_FROM_PTR(result);
  })


  MPY_BIND_LOCALS_DICT(matrix,
    MPY_BIND_ROM_PTR(rotate),
    MPY_BIND_ROM_PTR(rotate_radians),
    MPY_BIND_ROM_PTR(translate),
    MPY_BIND_ROM_PTR(scale),
    MPY_BIND_ROM_PTR(multiply),
    MPY_BIND_ROM_PTR(inverse),
  )

  MP_DEFINE_CONST_OBJ_TYPE(
      type_mat3,
      MP_QSTR_mat3,
      MP_TYPE_FLAG_NONE,
      make_new, (const void *)matrix_new,
      locals_dict, &matrix_locals_dict
  );

}




#include "mp_tracked_allocator.hpp"

#include "mp_helpers.hpp"
#include "picovector.hpp"

using namespace picovector;

bool mp_obj_is_vec2(mp_obj_t vec2_in) {
  return mp_obj_is_type(vec2_in, &type_vec2);
}

vec2_t mp_obj_get_vec2(mp_obj_t vec2_in) {
  if(mp_obj_is_vec2(vec2_in)) {
    vec2_obj_t *vec2 = (vec2_obj_t *)MP_OBJ_TO_PTR(vec2_in);
    return vec2->v;
  }
  mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid parameters, expected vec2(x, y)"));
}

vec2_t mp_obj_get_vec2_from_xy(const mp_obj_t *args) {
    int x = mp_obj_get_float(args[0]);
    int y = mp_obj_get_float(args[1]);
    return vec2_t(x, y);
}

extern "C" {

  #include "py/runtime.h"

  MPY_BIND_NEW(vec2, {
    vec2_obj_t *self = mp_obj_malloc_with_finaliser(vec2_obj_t, type);
    if(n_args != 2 && n_args != 0) {
      mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("invalid parameters, expected vec2() or vec2(x, y)"));
    }
    if(n_args == 2) {
      self->v.x = mp_obj_get_float(args[0]);
      self->v.y = mp_obj_get_float(args[1]);
    }
    return MP_OBJ_FROM_PTR(self);
  })

  MPY_BIND_VAR(2, transform, {
    vec2_obj_t *self = (vec2_obj_t *)MP_OBJ_TO_PTR(args[0]);
    mat3_obj_t *t = (mat3_obj_t *)MP_OBJ_TO_PTR(args[1]);
    self->v = self->v.transform(t->m);
    return mp_const_none;
  })

  MPY_BIND_ATTR(vec2, {
    self(self_in, vec2_obj_t);

    action_t action = m_attr_action(dest);

    switch(attr | action) {
      case MP_QSTR_x | GET:
        dest[0] = mp_obj_new_float(self->v.x);
        return;

      case MP_QSTR_x | SET:
        self->v.x = mp_obj_get_float(dest[1]);
        dest[0] = MP_OBJ_NULL;
        return;

      case MP_QSTR_y | GET:
        dest[0] = mp_obj_new_float(self->v.y);
        return;

      case MP_QSTR_y | SET:
        self->v.y = mp_obj_get_float(dest[1]);
        dest[0] = MP_OBJ_NULL;
        return;
    };

    dest[1] = MP_OBJ_SENTINEL;
  })

  mp_obj_t make_vec2(vec2_t p) {
    vec2_obj_t *result = (vec2_obj_t*)mp_obj_malloc(vec2_obj_t, &type_vec2);
    result->v = p;
    return MP_OBJ_FROM_PTR(result);
  }

  static mp_obj_t vec2_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    vec2_obj_t *lhs = (vec2_obj_t*)MP_OBJ_TO_PTR(lhs_in);

    switch (op) {
      case MP_BINARY_OP_ADD: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return make_vec2(lhs->v + rhs->v);
        }
      }break;

      case MP_BINARY_OP_SUBTRACT: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return make_vec2(lhs->v - rhs->v);
        }
      }break;

      case MP_BINARY_OP_MULTIPLY: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return make_vec2(lhs->v * rhs->v);
        }
        if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
          float v = mp_obj_get_float(rhs_in);
          return make_vec2(lhs->v * v);
        }
      }break;

      case MP_BINARY_OP_TRUE_DIVIDE: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return make_vec2(lhs->v / rhs->v);
        }
        if (mp_obj_is_int(rhs_in) || mp_obj_is_float(rhs_in)) {
          float v = mp_obj_get_float(rhs_in);
          return make_vec2(lhs->v / v);
        }
      }break;

      case MP_BINARY_OP_EQUAL: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return mp_obj_new_bool(lhs->v == rhs->v);
        }
        return mp_const_false;
      }

      case MP_BINARY_OP_NOT_EQUAL: {
        if(mp_obj_is_type(rhs_in, &type_vec2)) {
          vec2_obj_t *rhs = (vec2_obj_t*)MP_OBJ_TO_PTR(rhs_in);
          return mp_obj_new_bool(lhs->v != rhs->v);
        }
        return mp_const_true;
      }

      default: {
        return MP_OBJ_NULL;
      }
    }

    return MP_OBJ_NULL;
  }

  static void vec2_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    self(self_in, vec2_obj_t);
    mp_printf(print, "vec(%f, %f)", self->v.x, self->v.y);
  }

  MPY_BIND_LOCALS_DICT(vec2,
    MPY_BIND_ROM_PTR(transform),
  )

  MP_DEFINE_CONST_OBJ_TYPE(
      type_vec2,
      MP_QSTR_vec2,
      MP_TYPE_FLAG_NONE,
      make_new, (const void *)vec2_new,
      print, (const void*)vec2_print,
      binary_op, (const void *)vec2_binary_op,
      attr, (const void *)vec2_attr,
      locals_dict, &vec2_locals_dict
  );

}



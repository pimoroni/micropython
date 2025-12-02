
#include "mp_tracked_allocator.hpp"
#include "../picovector.hpp"
#include "../primitive.hpp"
#include "../shape.hpp"
#include "../image.hpp"
#include "matrix.hpp"

#include "mp_helpers.hpp"

using namespace picovector;

extern "C" {

  #include "py/runtime.h"

  extern const mp_obj_type_t type_Shape;
  extern const mp_obj_type_t type_Shapes;

  typedef struct _shape_obj_t {
    mp_obj_base_t base;
    shape_t *shape;
  } shape_obj_t;

  mp_obj_t shape__del__(mp_obj_t self_in) {
    self(self_in, shape_obj_t);
    m_del_class(shape_t, self->shape);
    return mp_const_none;
  }

  mp_obj_t shapes_custom(size_t n_args, const mp_obj_t *args) {
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);

    for (size_t arg_i = 0; arg_i < n_args; arg_i++) {
      mp_obj_t path_obj = args[arg_i];

      if(!mp_obj_is_type(path_obj, &mp_type_list)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected a list of tuples"));
      }

      size_t count;
      mp_obj_t *path;
      mp_obj_list_get(path_obj, &count, &path);

      path_t poly(count);

      for(size_t i = 0; i < count; i++) {
        mp_obj_t point_obj = path[i];

        if(!mp_obj_is_type(point_obj, &mp_type_tuple)) {
          mp_raise_TypeError(MP_ERROR_TEXT("list elements must be tuples"));
        }

        size_t tuple_len;
        mp_obj_t *tuple_items;
        mp_obj_tuple_get(point_obj, &tuple_len, &tuple_items);

        if (tuple_len != 2) {
          mp_raise_ValueError(MP_ERROR_TEXT("tuples must contain (x, y) coordinates"));
        }

        // Extract elements; assuming ints here
        float x = mp_obj_get_float(tuple_items[0]);
        float y = mp_obj_get_float(tuple_items[1]);

        poly.add_point(x, y);
      }

      shape->shape->add_path(poly);
    }

    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_regular_polygon(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float r = mp_obj_get_float(pos_args[2]);
    int s = mp_obj_get_float(pos_args[3]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = regular_polygon(x, y, s, r);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_circle(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float r = mp_obj_get_float(pos_args[2]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = circle(x, y, r);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_rectangle(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float w = mp_obj_get_float(pos_args[2]);
    float h = mp_obj_get_float(pos_args[3]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = rectangle(x, y, w, h);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_rounded_rectangle(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float w = mp_obj_get_float(pos_args[2]);
    float h = mp_obj_get_float(pos_args[3]);
    float r1 = mp_obj_get_float(pos_args[4]);
    float r2 = r1, r3 = r1, r4 = r1;
    if(n_args >= 6) { r2 = mp_obj_get_float(pos_args[5]); }
    if(n_args >= 7) { r3 = mp_obj_get_float(pos_args[6]); }
    if(n_args >= 8) { r4 = mp_obj_get_float(pos_args[7]); }

    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = rounded_rectangle(x, y, w, h, r1, r2, r3, r4);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_squircle(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float s = mp_obj_get_float(pos_args[2]);
    float n = mp_obj_get_float(pos_args[3]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = squircle(x, y, s, n);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_arc(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float r = mp_obj_get_float(pos_args[2]);
    float f = mp_obj_get_float(pos_args[3]);
    float t = mp_obj_get_float(pos_args[4]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = arc(x, y, f, t, r);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_pie(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float r = mp_obj_get_float(pos_args[2]);
    float f = mp_obj_get_float(pos_args[3]);
    float t = mp_obj_get_float(pos_args[4]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = pie(x, y, f, t, r);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_star(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    int s = mp_obj_get_float(pos_args[2]);
    float ro = mp_obj_get_float(pos_args[3]);
    float ri = mp_obj_get_float(pos_args[4]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = star(x, y, s, ro, ri);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_line(size_t n_args, const mp_obj_t *pos_args) {
    float x1 = mp_obj_get_float(pos_args[0]);
    float y1 = mp_obj_get_float(pos_args[1]);
    float x2 = mp_obj_get_float(pos_args[2]);
    float y2 = mp_obj_get_float(pos_args[3]);
    float w = mp_obj_get_float(pos_args[4]);
    shape_obj_t *shape = mp_obj_malloc_with_finaliser(shape_obj_t, &type_Shape);
    shape->shape = line(x1, y1, x2, y2, w);
    return MP_OBJ_FROM_PTR(shape);
  }

  mp_obj_t shapes_stroke(size_t n_args, const mp_obj_t *pos_args) {
    const shape_obj_t *self = (shape_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    float width = mp_obj_get_float(pos_args[1]);
    self->shape->stroke(width);
    return MP_OBJ_FROM_PTR(self);
  }

  static void shape_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    self(self_in, shape_obj_t);

    if(attr == MP_QSTR_transform) { // get
      if(dest[0] == MP_OBJ_NULL) {
        matrix_obj_t *out = mp_obj_malloc_with_finaliser(matrix_obj_t, &type_Matrix);
        out->m = self->shape->transform;
        dest[0] = MP_OBJ_FROM_PTR(out);
        return;
      }

      if(dest[1] != MP_OBJ_NULL && dest[0] != MP_OBJ_NULL) { // set
        if(!mp_obj_is_type(dest[1], &type_Matrix)) {
          mp_raise_TypeError(MP_ERROR_TEXT("expected Matrix"));
        }
        matrix_obj_t *in = (matrix_obj_t *)MP_OBJ_TO_PTR(dest[1]);
        self->shape->transform = in->m;
        dest[0] = MP_OBJ_NULL;
        return;
      }
    }

    dest[1] = MP_OBJ_SENTINEL;
  }

  /*
    micropython bindings
  */
  static MP_DEFINE_CONST_FUN_OBJ_1(shape__del___obj, shape__del__);
  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_stroke_obj, 1, shapes_stroke);

  static const mp_rom_map_elem_t shape_locals_dict_table[] = {
      { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&shape__del___obj) },
      { MP_ROM_QSTR(MP_QSTR_stroke), MP_ROM_PTR(&shapes_stroke_obj) },
  };
  static MP_DEFINE_CONST_DICT(shape_locals_dict, shape_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
      type_Shape,
      MP_QSTR_Shape,
      MP_TYPE_FLAG_NONE,
      attr, (const void *)shape_attr,
      locals_dict, &shape_locals_dict
  );

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_custom_obj, 1, shapes_custom);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_custom_static_obj, MP_ROM_PTR(&shapes_custom_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_regular_polygon_obj, 4, shapes_regular_polygon);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_regular_polygon_static_obj, MP_ROM_PTR(&shapes_regular_polygon_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_rectangle_obj, 4, shapes_rectangle);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_rectangle_static_obj, MP_ROM_PTR(&shapes_rectangle_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_rounded_rectangle_obj, 5, shapes_rounded_rectangle);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_rounded_rectangle_static_obj, MP_ROM_PTR(&shapes_rounded_rectangle_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_squircle_obj, 4, shapes_squircle);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_squircle_static_obj, MP_ROM_PTR(&shapes_squircle_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_circle_obj, 3, shapes_circle);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_circle_static_obj, MP_ROM_PTR(&shapes_circle_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_arc_obj, 4, shapes_arc);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_arc_static_obj, MP_ROM_PTR(&shapes_arc_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_pie_obj, 4, shapes_pie);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_pie_static_obj, MP_ROM_PTR(&shapes_pie_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_star_obj, 4, shapes_star);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_star_static_obj, MP_ROM_PTR(&shapes_star_obj));

  static MP_DEFINE_CONST_FUN_OBJ_VAR(shapes_line_obj, 5, shapes_line);
  static MP_DEFINE_CONST_STATICMETHOD_OBJ(shapes_line_static_obj, MP_ROM_PTR(&shapes_line_obj));


  static const mp_rom_map_elem_t shapes_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_custom), MP_ROM_PTR(&shapes_custom_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_regular_polygon), MP_ROM_PTR(&shapes_regular_polygon_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_squircle), MP_ROM_PTR(&shapes_squircle_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&shapes_circle_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_rectangle), MP_ROM_PTR(&shapes_rectangle_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_rounded_rectangle), MP_ROM_PTR(&shapes_rounded_rectangle_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&shapes_arc_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_pie), MP_ROM_PTR(&shapes_pie_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_star), MP_ROM_PTR(&shapes_star_static_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&shapes_line_static_obj) },

  };
  static MP_DEFINE_CONST_DICT(shapes_locals_dict, shapes_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
      type_Shapes,
      MP_QSTR_shapes,
      MP_TYPE_FLAG_NONE,
      locals_dict, &shapes_locals_dict
  );


}

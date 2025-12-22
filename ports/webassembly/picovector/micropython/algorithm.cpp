#include "mp_helpers.hpp"
#include "picovector.hpp"

#include "../algorithms/algorithms.hpp"

extern "C" {
  #include "py/runtime.h"

  MPY_BIND_STATICMETHOD_VAR(3, clip_line, {
    point_obj_t *p1 = (point_obj_t *)MP_OBJ_TO_PTR(args[0]);
    point_obj_t *p2 = (point_obj_t *)MP_OBJ_TO_PTR(args[1]);
    const rect_obj_t *r = (rect_obj_t *)MP_OBJ_TO_PTR(args[2]);

    bool result = clip_line(p1->point, p2->point, r->rect);
    return mp_obj_new_bool(result);
  })

  MPY_BIND_STATICMETHOD_VAR(2, dda, {
    point_obj_t *p = (point_obj_t *)MP_OBJ_TO_PTR(args[0]);
    point_obj_t *v = (point_obj_t *)MP_OBJ_TO_PTR(args[1]);
    int max = mp_obj_get_int(args[2]);

    mp_obj_t result = mp_obj_new_list(max, NULL);
    int i = 0;

    dda(p->point, v->point, [&result, &i, max](float hit_x, float hit_y, int gx, int gy, int edge, float offset, float distance) -> bool {
      // add hit to result and return false when done
      mp_obj_tuple_t *t = (mp_obj_tuple_t*)MP_OBJ_TO_PTR(mp_obj_new_tuple(5, NULL));

      point_obj_t *p = mp_obj_malloc_with_finaliser(point_obj_t, &type_point);
      p->point.x = hit_x;
      p->point.y = hit_y;

      point_obj_t *g = mp_obj_malloc_with_finaliser(point_obj_t, &type_point);
      g->point.x = gx;
      g->point.y = gy;

      t->items[0] = MP_OBJ_FROM_PTR(p);
      t->items[1] = MP_OBJ_FROM_PTR(g);
      t->items[2] = mp_obj_new_int(edge);
      t->items[3] = mp_obj_new_float(offset);
      t->items[4] = mp_obj_new_float(distance);

      mp_obj_list_store(result, mp_obj_new_int(i), MP_OBJ_FROM_PTR(t));
      i++;
      return i < max;
    });

    return result;
  })


  MPY_BIND_LOCALS_DICT(algorithm,
    MPY_BIND_ROM_PTR_STATIC(clip_line),
    MPY_BIND_ROM_PTR_STATIC(dda),
  )


  MP_DEFINE_CONST_OBJ_TYPE(
      type_algorithm,
      MP_QSTR_algorithm,
      MP_TYPE_FLAG_NONE,
      locals_dict, &algorithm_locals_dict
  );


}



#include "mp_tracked_allocator.hpp"
// #include "../picovector.hpp"
// #include "../primitive.hpp"
// #include "../image.hpp"


#include "brush.hpp"
#include "shape.hpp"
#include "font.hpp"
#include "pixel_font.hpp"
#include "image.hpp"
#include "input.hpp"
#include "matrix.hpp"

#include "mp_helpers.hpp"

using namespace picovector;

extern "C" {

  #include "py/runtime.h"

  int screen_width = 160;
  int screen_height = 120;
  uint32_t framebuffer[160 * 120];

#ifndef PICO
  int debug_width = 300;
  int debug_height = 360;
  uint32_t debug_buffer[300 * 360];
#endif

  mp_obj_t modpicovector___init__(void) {
      return mp_const_none;
  }


  mp_obj_t modpicovector_dda(size_t n_args, const mp_obj_t *pos_args) {
    float x = mp_obj_get_float(pos_args[0]);
    float y = mp_obj_get_float(pos_args[1]);
    float dx = mp_obj_get_float(pos_args[2]);
    float dy = mp_obj_get_float(pos_args[3]);

    size_t map_height;
    mp_obj_t *map_rows;
    mp_obj_get_array(pos_args[4], &map_height, &map_rows);
    size_t map_width = mp_obj_get_int(mp_obj_len(map_rows[0]));

    int max = mp_obj_get_int(pos_args[5]);

    int ix = floor(x);
    int iy = floor(y);

    const float eps = 1e-30f;
    float inv_dx = fabs(dx) > eps ? 1.0f / dx : 1e30f;
    float inv_dy = fabs(dy) > eps ? 1.0f / dy : 1e30f;

    int step_x = (dx > 0.0f) ? 1 : (dx < 0.0f ? -1 : 0);
    int step_y = (dy > 0.0f) ? 1 : (dy < 0.0f ? -1 : 0);

    float t_delta_x = fabs(inv_dx);
    float t_delta_y = fabs(inv_dy);

    float t_max_x = 1e30f;
    float t_max_y = 1e30f;

    if(step_x > 0) {
      t_max_x = ((float(ix) + 1.0f) - x) * inv_dx;
    } else if(step_x < 0) {
      t_max_x = ((float(ix) - x)) * inv_dx;
    }

    if(step_y > 0) {
      t_max_y = ((float(iy) + 1.0f) - y) * inv_dy;
    } else if(step_y < 0) {
      t_max_y = ((float(iy) - y)) * inv_dy;
    }

    float t_enter = 0.0f;

    mp_obj_t result = mp_obj_new_list(0, NULL);

    //mp_obj_t result = mp_obj_new_list(0, NULL);

    int i = 0;
    while (t_enter <= max) {
      float t_exit = std::min(t_max_x, t_max_y);

      // calculate the intersection position
      float hit_x = x + dx * t_exit;
      float hit_y = y + dy * t_exit;

      // calculate the edge which the intersection occured on (0=top, 1=right, 2=bottom, 3=left)
      bool vertical = abs(hit_y - round(hit_y)) > abs(hit_x - round(hit_x));
      int edge = vertical ? (dx < 0 ? 3 : 1) : (dy < 0 ? 0 : 2);

      // calculate the intersection offset
      float offset = vertical ? (hit_y - floor(hit_y)) : (hit_x - floor(hit_x));

      float distance = sqrt(pow(hit_x - x, 2) + pow(hit_y - y, 2));

      // calculate grid square of intersection
      int gx, gy;
      if(vertical) {
        gx = int(hit_x + (edge == 3 ? -0.5f : 0.5f));
        gy = int(hit_y);
      } else {
        gx = int(hit_x);
        gy = int(hit_y + (edge == 0 ? -0.5f : 0.5f));
      }

      if(gy < 0 || gy >= int(map_height)) {
        break;
      }

      size_t row_width;
      mp_obj_t *map_row;
      mp_obj_get_array(map_rows[gy], &row_width, &map_row);

      if(gx < 0 || gx >= int(row_width)) {
        break;
      }

      int tile_id = mp_obj_get_int(map_row[gx]);

      if(tile_id > 0) {
        // if solid then create intersection entry
        mp_obj_tuple_t *t = (mp_obj_tuple_t*)MP_OBJ_TO_PTR(mp_obj_new_tuple(8, NULL));
        t->items[0] = mp_obj_new_float(hit_x);
        t->items[1] = mp_obj_new_float(hit_y);
        t->items[2] = mp_obj_new_int(gx);
        t->items[3] = mp_obj_new_int(gy);
        t->items[4] = mp_obj_new_int(edge);
        t->items[5] = mp_obj_new_float(offset);
        t->items[6] = mp_obj_new_float(distance);
        t->items[7] = mp_obj_new_int(tile_id);

        mp_obj_list_append(result, MP_OBJ_FROM_PTR(t));

        i++;
        if(i >= max) {
          break;
        }
      }

      // Step to the next cell: whichever boundary we hit first
      if (t_max_x < t_max_y) {
        ix += step_x;
        t_enter = t_max_x;
        t_max_x += t_delta_x; // next vertical boundary
      } else {
        iy += step_y;
        t_enter = t_max_y;
        t_max_y += t_delta_y; // next horizontal boundary
      }
    }

    return MP_OBJ_FROM_PTR(result);
  }


  void modpicovector_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] == MP_OBJ_NULL) {
      if (attr == MP_QSTR_screen) {
        image_obj_t *image = mp_obj_malloc_with_finaliser(image_obj_t, &type_Image);
        image->image = new(m_malloc(sizeof(image_t))) image_t(framebuffer, screen_width, screen_height);
        dest[0] = MP_OBJ_FROM_PTR(image);
      }
#ifndef PICO
      if (attr == MP_QSTR_debug) {
        image_obj_t *image = mp_obj_malloc_with_finaliser(image_obj_t, &type_Image);
        image->image = new(m_malloc(sizeof(image_t))) image_t(debug_buffer, debug_width, debug_height);
        dest[0] = MP_OBJ_FROM_PTR(image);
      }
#endif
    }
  }

}

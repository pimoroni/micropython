#include <algorithm>
#include "algorithms.hpp"

namespace picovector {

  void dda(point_t p, point_t v, dda_callback_t cb) {
    int ix = floor(p.x);
    int iy = floor(p.y);

    const float eps = 1e-30f;
    float inv_dx = fabs(v.x) > eps ? 1.0f / v.x : 1e30f;
    float inv_dy = fabs(v.y) > eps ? 1.0f / v.y : 1e30f;

    int step_x = (v.x > 0.0f) ? 1 : (v.x < 0.0f ? -1 : 0);
    int step_y = (v.y > 0.0f) ? 1 : (v.y < 0.0f ? -1 : 0);

    float t_delta_x = fabs(inv_dx);
    float t_delta_y = fabs(inv_dy);

    float t_max_x = 1e30f;
    float t_max_y = 1e30f;

    if(step_x > 0) {
      t_max_x = ((float(ix) + 1.0f) - p.x) * inv_dx;
    } else if(step_x < 0) {
      t_max_x = ((float(ix) - p.x)) * inv_dx;
    }

    if(step_y > 0) {
      t_max_y = ((float(iy) + 1.0f) - p.y) * inv_dy;
    } else if(step_y < 0) {
      t_max_y = ((float(iy) - p.y)) * inv_dy;
    }

    int i = 0;
    while (true) {
      float t_exit = std::min(t_max_x, t_max_y);

      // calculate the intersection position
      float hit_x = p.x + v.x * t_exit;
      float hit_y = p.y + v.y * t_exit;

      // calculate the edge which the intersection occured on (0=top, 1=right, 2=bottom, 3=left)
      bool vertical = abs(hit_x - round(hit_x)) < abs(hit_y - round(hit_y));
      int edge = vertical ? (v.x > 0 ? 3 : 1) : (v.y > 0 ? 0 : 2);

      // calculate the intersection offset
      float offset = vertical ? (hit_y - floor(hit_y)) : (hit_x - floor(hit_x));

      float distance = sqrt(pow(hit_x - p.x, 2) + pow(hit_y - p.y, 2));

      // calculate grid square of intersection
      int gx, gy;
      if(vertical) {
        gx = floor(hit_x + (v.x > 0.0f ? 0.5f : -0.5f));
        gy = floor(hit_y);
      } else {
        gx = floor(hit_x);
        gy = floor(hit_y + (v.y > 0.0f ? 0.5f : -0.5f));
      }

      if(!cb(hit_x, hit_y, gx, gy, edge, offset, distance)) {
        break;
      }

      // step to the next cell: whichever boundary we hit first
      if (t_max_x < t_max_y) {
        ix += step_x;
        t_max_x += t_delta_x; // next vertical boundary
      } else {
        iy += step_y;
        t_max_y += t_delta_y; // next horizontal boundary
      }
    }
  }

}
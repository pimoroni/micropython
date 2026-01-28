
#include "algorithms.hpp"

namespace picovector {

  bool clip_line(vec2_t &p1, vec2_t &p2, const rect_t r) {
    vec2_t v1 = p1;
    vec2_t v2 = p2;

    enum {CS_LEFT = 1, CS_RIGHT = 2, CS_BOTTOM = 4, CS_TOP = 8};
    auto cs_outcode = [](const vec2_t &p, const rect_t &b) {
      // determine cohen-sutherland outcode for vec2
      int code = 0;
      if (p.x < b.x) code |= CS_LEFT;
      else if (p.x > b.x + b.w) code |= CS_RIGHT;
      if (p.y < b.y) code |= CS_BOTTOM;
      else if (p.y > b.y + b.h) code |= CS_TOP;
      return code;
    };

    int out1 = cs_outcode(v1, r);
    int out2 = cs_outcode(v2, r);

    while(true) {
      if(!(out1 | out2)) {
        p1 = v1; p2 = v2;
        return true;
      }else if(out1 & out2) {
        return false; // no part of line segment within bounds
      }else{
        // at least one endvec2 is outside; pick it
        int out = out1 ? out1 : out2;
        float x, y;
        float dx = v2.x - v1.x, dy = v2.y - v1.y;

        if(out & CS_TOP) {
            y = r.y + r.h; x = v1.x + dx * ((r.y + r.h) - v1.y) / dy;
        } else if (out & CS_BOTTOM) {
            y = r.y; x = v1.x + dx * (r.y - v1.y) / dy;
        } else if (out & CS_RIGHT) {
            x = r.x + r.w; y = v1.y + dy * ((r.x + r.w) - v1.x) / dx;
        } else { // CS_LEFT
            x = r.x; y = v1.y + dy * (r.x - v1.x) / dx;
        }

        if (out == out1) {
          v1.x = x; v1.y = y;
          out1 = cs_outcode(v1, r);
        } else {
          v2.x = x; v2.y = y;
          out2 = cs_outcode(v2, r);
        }
      }
    }
  }
}
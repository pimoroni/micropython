#pragma once

#include <stdint.h>
#include <algorithm>

#include "point.hpp"

using std::max;
using std::min;

namespace picovector {

  class rect_t {
  public:
    float x;
    float y;
    float w;
    float h;

    rect_t() {}

    rect_t(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    rect_t(const point_t &p1, const point_t &p2) {
      x = p1.x; y = p1. y; w = p2.x - p1.x; h = p2.y - p1.y;
    }

    point_t tl() {return point_t(x, y);}
    point_t br() {return point_t(x + w, y + h);}

    void offset(point_t p) {
      x += p.x;
      y += p.y;
    }

    void offset(int ox, int oy) {
      x += ox;
      y += oy;
    }

    bool operator==(const rect_t &rhs) const {
      return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
    }

    bool empty() {
      return w == 0 || h == 0;
    }

    bool contains(const point_t &p) {
      return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }

    bool contains(const rect_t &o) {
      return o.x >= x && o.y >= y && (o.x + o.w) <= (x + w) && (o.y + o.h) < (y + h);
    }

    rect_t normalise() {
      rect_t n = *this;

      if(n.w < 0) {
        n.x += n.w;
        n.w = -n.w;
      }

      if(n.h < 0) {
        n.y += n.h;
        n.h = -n.h;
      }

      return n;
    }

    rect_t intersection(rect_t r) {
      rect_t rn = r.normalise();
      rect_t tn = this->normalise();

      // Compute the edges of the intersection
      float x1 = max(tn.x, rn.x);
      float y1 = max(tn.y, rn.y);
      float x2 = min(tn.x + tn.w, rn.x + rn.w);
      float y2 = min(tn.y + tn.h, rn.y + rn.h);

      if (x1 < x2 && y1 < y2) {
        return rect_t(x1, y1, x2 - x1, y2 - y1);
      }

      return rect_t(0, 0, 0, 0);
    }

    bool intersects(const rect_t &r) {
      rect_t i = this->intersection(r);
      return !i.empty();
    }

    void shrink(int a) {
      x += a;
      y += a;
      w -= a + a;
      h -= a + a;
    }

    void shrink(float left, float top, float right, float bottom) {
      x += left;
      y += top;
      w -= left + right;
      h -= top + bottom;
    }

    void debug(std::string l = "?") {
      printf("%s: %f, %f (%f x %f)\n", l.c_str(), x, y, w, h);
    }
  };

}
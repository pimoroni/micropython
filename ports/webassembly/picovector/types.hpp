#pragma once

#include <stdint.h>
#include <algorithm>

#include "mat3.hpp"

using std::max;
using std::min;

namespace picovector {

  typedef int32_t fx16_t; // fixed point 16:16 type

  struct point_t {
    float x;
    float y;

    point_t() {}
    point_t(float x, float y) : x(x), y(y) {}

    bool operator==(const point_t &rhs) const {
      return x == rhs.x && y == rhs.y;
    }

    point_t transform(mat3_t *t) {
      if(!t) {return *this;}
      return point_t(
        t->v00 * x + t->v01 * y + t->v02,
        t->v10 * x + t->v11 * y + t->v12
      );
    }

    point_t transform(const mat3_t &t) {
      return point_t(
        t.v00 * x + t.v01 * y + t.v02,
        t.v10 * x + t.v11 * y + t.v12
      );
    }
  };

  static inline fx16_t f_to_fx16(float v) {
    return fx16_t(v * 65536.0f);
  }

  struct fx16_point_t {
    fx16_t x;
    fx16_t y;

    fx16_point_t(fx16_t x, fx16_t y) : x(x), y(y) {}

    fx16_point_t(float fx, float fy) {
      x = f_to_fx16(fx);
      y = f_to_fx16(fy);
    }

    bool operator==(const point_t &rhs) const {
      return x == rhs.x && y == rhs.y;
    }

    fx16_point_t transform(mat3_t *t) {
      if(!t) {return *this;}
      return fx16_point_t(
        t->v00 * x + t->v01 * y + t->v02,
        t->v10 * x + t->v11 * y + t->v12
      );
    }

    fx16_point_t transform(const mat3_t &t) {
      return fx16_point_t(
        t.v00 * x + t.v01 * y + t.v02,
        t.v10 * x + t.v11 * y + t.v12
      );
    }
  };

  struct rect_t {
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

    rect_t intersection(const rect_t &r) const {
      rect_t rn = r;
      rn.normalise();
      rect_t tn = *this;
      tn.normalise();

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

    void inflate(float a) {
      this->inflate(a, a, a, a);
    }

    void inflate(float left, float top, float right, float bottom) {
      x -= left;
      y -= top;
      w += left + right;
      h += top + bottom;
    }

    void deflate(float a) {
      this->deflate(a, a, a, a);
    }

    void deflate(float left, float top, float right, float bottom) {
      x += left;
      y += top;
      w -= left + right;
      h -= top + bottom;
    }
  };

}
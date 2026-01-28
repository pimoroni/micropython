#pragma once

#include <stdint.h>
#include <algorithm>

#include "mat3.hpp"

using std::max;
using std::min;

namespace picovector {

  typedef int32_t fx16_t; // fixed point 16:16 type

  struct vec2_t {
    float x;
    float y;

    vec2_t() {}
    vec2_t(float x, float y) : x(x), y(y) {}

    bool operator==(const vec2_t &rhs) const {
      return x == rhs.x && y == rhs.y;
    }


    vec2_t& operator+=(const vec2_t& rhs) {x += rhs.x; y += rhs.y; return *this;}
    vec2_t& operator-=(const vec2_t& rhs) {x -= rhs.x; y -= rhs.y; return *this;}
    vec2_t& operator*=(const vec2_t& rhs) {x *= rhs.x; y *= rhs.y; return *this;}
    vec2_t& operator*=(const float rhs) {x *= rhs; y *= rhs; return *this;}
    vec2_t& operator/=(const vec2_t& rhs) {x /= rhs.x; y /= rhs.y; return *this;}
    vec2_t& operator/=(const float rhs) {x /= rhs; y /= rhs; return *this;}

    friend vec2_t operator+(vec2_t lhs, const vec2_t& rhs) { lhs += rhs; return lhs; }
    friend vec2_t operator-(vec2_t lhs, const vec2_t& rhs) { lhs -= rhs; return lhs; }
    friend vec2_t operator*(vec2_t lhs, const vec2_t& rhs) { lhs *= rhs; return lhs; }
    friend vec2_t operator*(vec2_t lhs, const float rhs) { lhs *= rhs; return lhs; }
    friend vec2_t operator/(vec2_t lhs, const vec2_t& rhs) { lhs /= rhs; return lhs; }
    friend vec2_t operator/(vec2_t lhs, const float rhs) { lhs /= rhs; return lhs; }

    vec2_t operator+() const { return *this; }
    vec2_t operator-() const { return vec2_t(-x, -y); }

    friend bool operator!=(const vec2_t& a, const vec2_t& b) { return !(a == b); }

    vec2_t transform(mat3_t *t) {
      if(!t) {return *this;}
      return vec2_t(
        t->v00 * x + t->v01 * y + t->v02,
        t->v10 * x + t->v11 * y + t->v12
      );
    }

    vec2_t transform(const mat3_t &t) {
      return vec2_t(
        t.v00 * x + t.v01 * y + t.v02,
        t.v10 * x + t.v11 * y + t.v12
      );
    }
  };

  static inline fx16_t f_to_fx16(float v) {
    return fx16_t(v * 65536.0f);
  }

  struct fx16_vec2_t {
    fx16_t x;
    fx16_t y;

#ifdef PICO
    // On Pico "fx16_t" and by extension "int32_t" is type "long"
    fx16_vec2_t(int x, int y) : x(x), y(y) {}
#endif
    fx16_vec2_t(fx16_t x, fx16_t y) : x(x), y(y) {}

    fx16_vec2_t(float fx, float fy) {
      x = f_to_fx16(fx);
      y = f_to_fx16(fy);
    }

    bool operator==(const vec2_t &rhs) const {
      return x == rhs.x && y == rhs.y;
    }

    fx16_vec2_t transform(mat3_t *t) {
      if(!t) {return *this;}
      return fx16_vec2_t(
        t->v00 * x + t->v01 * y + t->v02,
        t->v10 * x + t->v11 * y + t->v12
      );
    }

    fx16_vec2_t transform(const mat3_t &t) {
      return fx16_vec2_t(
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

    rect_t(const vec2_t &p1, const vec2_t &p2) {
      x = p1.x; y = p1. y; w = p2.x - p1.x; h = p2.y - p1.y;
    }

    vec2_t tl() {return vec2_t(x, y);}
    vec2_t br() {return vec2_t(x + w, y + h);}

    void offset(vec2_t p) {
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

    bool contains(const vec2_t &p) {
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

    rect_t round() {
      rect_t r;
      r.x = floorf(this->x);
      r.y = floorf(this->y);
      r.w = ceilf(this->w + this->x) - r.x;
      r.h = ceilf(this->h + this->y) - r.y;
      return r;
    }

    rect_t floor() {
      rect_t r;
      r.x = floorf(this->x);
      r.y = floorf(this->y);
      r.w = floorf(this->w + this->x) - r.x;
      r.h = floorf(this->h + this->y) - r.y;
      return r;
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

    rect_t transform(mat3_t *m) {
      vec2_t tl = vec2_t(this->x, this->y);
      vec2_t tr = vec2_t(this->x + this->w, this->y);
      vec2_t bl = vec2_t(this->x, this->y + this->h);
      vec2_t br = vec2_t(this->x + this->w, this->y + this->h);

      tl = tl.transform(m);
      tr = tr.transform(m);
      bl = bl.transform(m);
      br = br.transform(m);

      float minx = std::min(tl.x, std::min(tr.x, std::min(bl.x, br.x)));
      float miny = std::min(tl.y, std::min(tr.y, std::min(bl.y, br.y)));
      float maxx = std::max(tl.x, std::max(tr.x, std::max(bl.x, br.x)));
      float maxy = std::max(tl.y, std::max(tr.y, std::max(bl.y, br.y)));

      return rect_t(
        x = (int32_t)minx,
        y = (int32_t)miny,
        w = (int32_t)(maxx - minx),
        h = (int32_t)(maxy - miny)
      );
    }
  };

}
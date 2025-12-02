#pragma once

#include <stdint.h>
#include <algorithm>

#include "matrix.hpp"

namespace picovector {

  class point_t {
  public:  
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

}

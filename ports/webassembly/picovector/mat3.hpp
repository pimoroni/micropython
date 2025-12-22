#pragma once

#include <cmath>
#include <string.h>

namespace picovector {

  class mat3_t {
  public:
    float v00, v10, v20, v01, v11, v21, v02, v12, v22;

    mat3_t() {
      memset(this, 0, sizeof(mat3_t));
      v00 = v11 = v22 = 1.0f;
    }

    mat3_t& rotate(float a) {
      return this->rotate_radians(a * M_PI / 180.0f);
    }

    mat3_t& rotate_radians(float a) {
      mat3_t rotation;
      float c = cosf(a);
      float s = sinf(a);
      rotation.v00 = c; rotation.v01 = -s; rotation.v10 = s; rotation.v11 = c;
      return this->multiply(rotation);
    }

    mat3_t& translate(float x, float y) {
      mat3_t translation;
      translation.v02 = x; translation.v12 = y;
      return this->multiply(translation);
    }

    mat3_t& scale(float v) {
      return this->scale(v, v);
    }

    mat3_t& scale(float x, float y) {
      mat3_t scale;
      scale.v00 = x; scale.v11 = y;
      return this->multiply(scale);
    }

    mat3_t& multiply(const mat3_t &m) {
      mat3_t r;
      r.v00 = v00 * m.v00 + v01 * m.v10 + v02 * m.v20;
      r.v01 = v00 * m.v01 + v01 * m.v11 + v02 * m.v21;
      r.v02 = v00 * m.v02 + v01 * m.v12 + v02 * m.v22;
      r.v10 = v10 * m.v00 + v11 * m.v10 + v12 * m.v20;
      r.v11 = v10 * m.v01 + v11 * m.v11 + v12 * m.v21;
      r.v12 = v10 * m.v02 + v11 * m.v12 + v12 * m.v22;
      r.v20 = v20 * m.v00 + v21 * m.v10 + v22 * m.v20;
      r.v21 = v20 * m.v01 + v21 * m.v11 + v22 * m.v21;
      r.v22 = v20 * m.v02 + v21 * m.v12 + v22 * m.v22;
      memcpy(this, &r, sizeof(mat3_t));
      return *this;
    }

    mat3_t& inverse() {
      mat3_t r;

       // Name elements for readability
      float m00 = v00, m01 = v01, m02 = v02;
      float m10 = v10, m11 = v11, m12 = v12;
      float m20 = v20, m21 = v21, m22 = v22;

      // Determinant
      float det =
          m00 * (m11 * m22 - m12 * m21) -
          m01 * (m10 * m22 - m12 * m20) +
          m02 * (m10 * m21 - m11 * m20);

      float inv_det = 1.0f / det;

      // Adjugate (transpose of cofactor matrix),
      // then multiply by 1/det to get inverse.
      this->v00 =  (m11 * m22 - m12 * m21) * inv_det;
      this->v01 = -(m01 * m22 - m02 * m21) * inv_det;
      this->v02 =  (m01 * m12 - m02 * m11) * inv_det;
      this->v10 = -(m10 * m22 - m12 * m20) * inv_det;
      this->v11 =  (m00 * m22 - m02 * m20) * inv_det;
      this->v12 = -(m00 * m12 - m02 * m10) * inv_det;
      this->v20 =  (m10 * m21 - m11 * m20) * inv_det;
      this->v21 = -(m00 * m21 - m01 * m20) * inv_det;
      this->v22 =  (m00 * m11 - m01 * m10) * inv_det;

      return *this;
    }
  };

}

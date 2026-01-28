#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "../picovector.hpp"
#include "../image.hpp"

namespace picovector {

  // One-pole lowpass step: y += (x - y) * k
  // k is Q16: 0..65535 ~ 0..(almost 1.0)
  static inline int iir_step_q16(int y, int x, uint32_t k_q16) {
      int diff = x - y;
      // y += diff * k
      y += (int)((diff * (int32_t)k_q16) >> 16);
      return y;
  }

  // Compute k in Q16 from a "radius"/strength parameter.
  // radius <= 0 => no blur.
  static inline uint32_t blur_k_from_radius_q16(float radius) {
      if (radius <= 0) return 0;

      // Mapping: a = exp(-1/(r+1)), k = 1-a
      // Bigger radius => smaller k => stronger smoothing.
      float r = (float)radius;
      float a = std::exp(-1.0f / (r + 1.0f));
      float k = 1.0f - a;

      // Clamp to [0, 0.9999] to avoid weirdness at extremes
      if (k < 0.0f) k = 0.0f;
      if (k > 0.9999f) k = 0.9999f;

      return (uint32_t)std::lround(k * 65536.0f);
  }


  void image_t::blur(float radius) {
    if (radius <= 0) return;

    const uint32_t k = blur_k_from_radius_q16(radius);
    if (k == 0) return;

    int width = int(_bounds.w);
    int height = int(_bounds.h);

    // ---- Horizontal pass: forward + backward (symmetric-ish) ----
    for (int y = 0; y < height; ++y) {
        uint8_t* row = (uint8_t*)_buffer + (size_t)y * (size_t)_row_stride;

        // Forward: left -> right
        int r = row[0], g = row[1], b = row[2], a = row[3];
        for (int x = 1; x < width; ++x) {
            uint8_t* p = row + x * 4;
            r = iir_step_q16(r, p[0], k);
            g = iir_step_q16(g, p[1], k);
            b = iir_step_q16(b, p[2], k);
            a = iir_step_q16(a, p[3], k);
            p[0] = (uint8_t)r;
            p[1] = (uint8_t)g;
            p[2] = (uint8_t)b;
            p[3] = (uint8_t)a;
        }

        // Backward: right -> left
        uint8_t* pr = row + (width - 1) * 4;
        r = pr[0]; g = pr[1]; b = pr[2]; a = pr[3];
        for (int x = width - 2; x >= 0; --x) {
            uint8_t* p = row + x * 4;
            r = iir_step_q16(r, p[0], k);
            g = iir_step_q16(g, p[1], k);
            b = iir_step_q16(b, p[2], k);
            a = iir_step_q16(a, p[3], k);
            p[0] = (uint8_t)r;
            p[1] = (uint8_t)g;
            p[2] = (uint8_t)b;
            p[3] = (uint8_t)a;
        }
    }

    // ---- Vertical pass: forward + backward (in-place, per column) ----
    for (int x = 0; x < width; ++x) {
        uint8_t* p0 = (uint8_t*)_buffer + x * 4;

        // Forward: top -> bottom
        int r = p0[0], g = p0[1], b = p0[2], a = p0[3];
        for (int y = 1; y < height; ++y) {
            uint8_t* p = (uint8_t*)_buffer + (size_t)y * (size_t)_row_stride + x * 4;
            r = iir_step_q16(r, p[0], k);
            g = iir_step_q16(g, p[1], k);
            b = iir_step_q16(b, p[2], k);
            a = iir_step_q16(a, p[3], k);
            p[0] = (uint8_t)r;
            p[1] = (uint8_t)g;
            p[2] = (uint8_t)b;
            p[3] = (uint8_t)a;
        }

        // Backward: bottom -> top
        uint8_t* pb = (uint8_t*)_buffer + (size_t)(height - 1) * (size_t)_row_stride + x * 4;
        r = pb[0]; g = pb[1]; b = pb[2]; a = pb[3];
        for (int y = height - 2; y >= 0; --y) {
            uint8_t* p = (uint8_t*)_buffer + (size_t)y * (size_t)_row_stride + x * 4;
            r = iir_step_q16(r, p[0], k);
            g = iir_step_q16(g, p[1], k);
            b = iir_step_q16(b, p[2], k);
            a = iir_step_q16(a, p[3], k);
            p[0] = (uint8_t)r;
            p[1] = (uint8_t)g;
            p[2] = (uint8_t)b;
            p[3] = (uint8_t)a;
        }
    }
  }

}
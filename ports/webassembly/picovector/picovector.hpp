#pragma once

#include "picovector.config.hpp"
#include <stdint.h>
#include <cassert>
#include <string.h>
#include <float.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <optional>

#ifndef PV_STD_ALLOCATOR
#define PV_STD_ALLOCATOR std::allocator
#endif

#ifndef PV_MALLOC
#define PV_MALLOC malloc
#endif

#ifndef PV_FREE
#define PV_FREE free
#endif

#ifndef PV_REALLOC
#define PV_REALLOC realloc
#endif

// TODO: bring back AA support
const size_t working_buffer_size = (50 + 20) * 1024;
extern char __attribute__((aligned(4))) PicoVector_working_buffer[working_buffer_size];


namespace picovector {

  #define debug_printf(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

  class brush_t;
  class image_t;
  class shape_t;
  class glyph_t;
  class mat3_t;

  struct _rspan {
    int x; // span start x
    int y; // span y
    int w; // span width in pixels
    int o; // opacity of the span for blending (used for AA only)

    _rspan() : x(0), y(0), w(0), o(0) {}
    _rspan(int x, int y, int w, int o = 255) : x(x), y(y), w(w), o(o) {}
  };

  void render(shape_t *shape, image_t *target, mat3_t *transform, brush_t *brush);
  void render_glyph(glyph_t *shape, image_t *target, mat3_t *transform, brush_t *brush);

}
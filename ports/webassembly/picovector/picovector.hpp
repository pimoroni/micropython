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

#ifdef PICO
extern char PicoVector_working_buffer[48156]; // On device
#else
extern char PicoVector_working_buffer[48240]; // macOS (emulator)
#endif

namespace picovector {

  #define debug_printf(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

  class brush_t;
  class image_t;
  class shape_t;
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

}
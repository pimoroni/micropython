#include "types.hpp"
#include "blend.hpp"

#include "brush.hpp"

namespace picovector {

  // empty implementations for unsupported modes
  void span_func_nop(image_t *target, brush_t *brush, int x, int y, int w) {}
  void masked_span_func_nop(image_t *target, brush_t *brush, int x, int y, int w, uint8_t *mask) {}


}
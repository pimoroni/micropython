#include "picovector.hpp"
#include "types.hpp"
#include "shape.hpp"
#include "image.hpp"
#include "mat3.hpp"
#include "brush.hpp"

namespace picovector {
  void pvr_reset();
  void pvr_add_path(vec2_t *p, int count, mat3_t *transform);
  void pvr_render(image_t *target, rect_t bounds, brush_t *brush);
}
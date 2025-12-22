#include <functional>
#include "../types.hpp"

namespace picovector {

  bool clip_line(point_t &p1, point_t &p2, const rect_t r);



  using dda_callback_t = std::function<bool(float, float, int, int, int, float, float)>;

  //typedef bool (*dda_callback_t)(float, float, int, int, int, float, float);
  void dda(point_t p, point_t v, dda_callback_t cb);
}
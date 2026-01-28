#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "../picovector.hpp"
#include "../image.hpp"

namespace picovector {

  void image_t::monochrome() {
    int width = _bounds.w;
    int height = _bounds.h;

    for(int y = 0; y < height; y++) {
      for(int x = 0; x < width; x++) {
        int offset = ((y * width) + x) << 2;
        uint8_t *p = (uint8_t*)(_buffer) + offset;

        // luminence with green bias (crude but fast)
        int pixel = (p[0] + (p[1] * 2) + p[2]) >> 2;
        p[0] = p[1] = p[2] = pixel;
      }
    }
  }

}
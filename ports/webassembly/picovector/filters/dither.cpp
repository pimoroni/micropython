#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "../picovector.hpp"
#include "../image.hpp"

namespace picovector {


  void image_t::dither() {
    uint8_t m[16] = {
      0, 136, 34, 170,
      204, 68, 238, 102,
      51, 187, 17, 153,
      255, 119, 221, 85
    };

    uint8_t ca[4] = {64, 191, 191, 255};
    uint8_t cb[4] = {0, 64, 64, 191};

    int width = _bounds.w;
    int height = _bounds.h;

    for(int y = 0; y < height; y++) {
      int y_lookup = (y & 0b11) << 2;
      for(int x = 0; x < width; x++) {
        int offset = ((y * width) + x) << 2;
        uint8_t *p = (uint8_t*)(_buffer) + offset;

        // luminence with green bias (crude but fast)
        int pixel = (p[0] + (p[1] * 2) + p[2]) >> 2;
        int scale = m[y_lookup | (x & 0b11)];

        int a = ca[pixel >> 6];
        int b = cb[pixel >> 6];

        if(pixel > (b + ((a - b) * scale >> 8))) {
          p[0] = p[1] = p[2] = a;
        }else{
          p[0] = p[1] = p[2] = b;
        }
      }
    }
  }

}
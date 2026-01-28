#pragma once

#include <cstdint>

#include "picovector.hpp"

namespace picovector {

  class color_t {
  public:
    uint32_t _p; // pre-multiplied r, g, b, a

  public:
    virtual ~color_t() = default;
    void premul(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  };

  class rgb_color_t : public color_t {
    uint8_t _r, _g, _b, _a;

  public:
    rgb_color_t(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    uint8_t r() const { return _r; }
    uint8_t g() const { return _g; }
    uint8_t b() const { return _b; }
    uint8_t a() const { return _a; }
  };

  class hsv_color_t : public color_t {
    uint8_t _h, _s, _v, _a;

  public:
    hsv_color_t(uint8_t h, uint8_t s, uint8_t v, uint8_t a);
    uint8_t h() const { return _h; }
    uint8_t s() const { return _s; }
    uint8_t v() const { return _v; }
    uint8_t a() const { return _a; }
  };

  class oklch_color_t : public color_t {
    float _l, _c, _h, _a;

  public:
    oklch_color_t(uint8_t l, uint8_t c, uint8_t h, uint8_t a);
    uint8_t l() const { return _l; }
    uint8_t c() const { return _c; }
    uint8_t h() const { return _h; }
    uint8_t a() const { return _a; }
  };

}
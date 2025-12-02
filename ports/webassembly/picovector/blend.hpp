#pragma once

#include <stdint.h>
#ifndef PICO
#define __not_in_flash_func(v) v
#endif

// TODO: this function probably doesn't belong here...?
inline uint32_t __not_in_flash_func(_make_col)(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  return __builtin_bswap32((r << 24) | (g << 16) | (b << 8) | a);
}

// TODO: consider making all images pre-multiplied alpha

// note: previously we had blend paths using the rp2 interpolators but these
// turn out to be slower due to mmio access times, often taking around 40%
// longer to do the same work

/*

  blending functions

*/

// blends a source rgba pixel over a destination rgba pixel
// (~30 cycles per pixel)
static inline __attribute__((always_inline))
void _blend_rgba_rgba(uint8_t *dst, uint8_t *src) {
  uint8_t sa = src[3]; // source alpha

  if(sa == 255) { // source fully opaque: overwrite
    *(uint32_t *)dst = *(const uint32_t *)src;
    return;
  }
  if(sa == 0) { // source fully transparent: skip
    return;
  }

  // blend r, g, b, and a channels
  dst[0] += (sa * (src[0] - dst[0])) >> 8;
  dst[1] += (sa * (src[1] - dst[1])) >> 8;
  dst[2] += (sa * (src[2] - dst[2])) >> 8;
  dst[3] = sa + ((dst[3] * (255 - sa)) >> 8);
}

// blends a source rgba pixel over a destination rgba pixel with alpha
// (~40 cycles per pixel)
static inline __attribute__((always_inline))
void _blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t a) {
  uint8_t sa = src[3]; // take copy of original source alpha
  uint16_t t = a * sa + 128; // combine source alpha with alpha
  src[3] = (t + (t >> 8)) >> 8;
  _blend_rgba_rgba(dst, src);
  src[3] = sa; // restore source alpha
}

// blends one rgba source pixel over a horizontal span of destination pixels
static inline __attribute__((always_inline))
void _span_blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint32_t w) {
  while(w--) {
    _blend_rgba_rgba(dst, src);
    dst += 4;
  }
}

// blends one rgba source pixel over a horizontal span of destination pixels with alpha mask
static inline __attribute__((always_inline))
void _span_blend_rgba_rgba_masked(uint8_t *dst, uint8_t *src, uint8_t *m, uint32_t w) {
  uint8_t sa = src[3]; // take copy of original source alpha
  while(w--) {
    uint16_t t = *m * sa + 128; // combine source alpha with mask alpha
    src[3] = (t + (t >> 8)) >> 8;
    _blend_rgba_rgba(dst, src);
    dst += 4;
    m++;
  }
  src[3] = sa; // revert the supplied value back
}

/*

  blitting functions

*/

// blends a horizontal run of rgba source pixels onto the corresponding destination pixels
static inline __attribute__((always_inline))
void _span_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint w, uint8_t a) {
  while(w--) {
    _blend_rgba_rgba(dst, src, a);
    dst += 4;
    src += 4;
  }
}

// blends a horizontal run of rgba source pixels from a palette onto the corresponding destination pixels
static inline __attribute__((always_inline))
void _span_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, uint w, uint8_t a) {
  while(w--) {
    _blend_rgba_rgba(dst, &pal[*src << 2], a);
    dst += 4;
    src++;
  }
}

static inline __attribute__((always_inline))
void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint x, int step, uint w, uint8_t a) {
  while(w--) {
    _blend_rgba_rgba(dst, src + ((x >> 16) << 2), a);
    dst += 4;
    x += step;
  }
}

static inline __attribute__((always_inline))
void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, uint x, int step, uint w, uint8_t a) {
  while(w--) {
    uint8_t i = *(src + (x >> 16));
    _blend_rgba_rgba(dst, &pal[i << 2], a);
    dst += 4;
    x += step;
  }
}
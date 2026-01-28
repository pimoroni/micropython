#pragma once

#include <stdint.h>

#include "color.hpp"

static inline __attribute__((always_inline))
uint32_t _r(const uint32_t c) {return c & 0xffu;}
static inline __attribute__((always_inline))
uint32_t _g(const uint32_t c) {return (c >> 8) & 0xffu;}
static inline __attribute__((always_inline))
uint32_t _b(const uint32_t c) {return (c >> 16) & 0xffu;}
static inline __attribute__((always_inline))
uint32_t _a(const uint32_t c) {return (c >> 24) & 0xffu;}

// takes a premultiplied packed color and applies alpha
static inline __attribute__((always_inline))
uint32_t _premul_mul_alpha(uint32_t c, uint32_t a) {
  uint32_t r = (_r(c) * a + 128) >> 8;
  uint32_t g = (_g(c) * a + 128) >> 8;
  uint32_t b = (_b(c) * a + 128) >> 8;
  a = (_a(c) * a + 128) >> 8;
  return r | (g << 8) | (b << 16) | (a << 24);
}

static inline __attribute__((always_inline))
uint32_t _premul_mul_alpha_channel(uint32_t c, uint32_t a) {
  return (c * a + 128) >> 8;
}

typedef uint32_t (*blend_func_t)(uint32_t dst, uint32_t r, uint32_t g, uint32_t b, uint32_t a);

static inline uint32_t blend_func_over(uint32_t dst, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    if (a == 0u)   return dst;
    if (a == 255u) return r | (g << 8) | (b << 16) | 0xff000000u;

    uint32_t dr =  dst        & 0xffu;
    uint32_t dg = (dst >>  8) & 0xffu;
    uint32_t db = (dst >> 16) & 0xffu;
    uint32_t da = (dst >> 24) & 0xffu;

    uint32_t inva = 255u - a;

    r += ((dr * inva + 128u) >> 8);
    g += ((dg * inva + 128u) >> 8);
    b += ((db * inva + 128u) >> 8);
    a += ((da * inva + 128u) >> 8);

    return r | (g << 8) | (b << 16) | (a << 24);
}

// // blends one rgba source pixel over a horizontal span of destination pixels
// static inline __attribute__((always_inline))
// void span_blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint32_t w) {
//   uint8_t sa = src[3];

//   if(sa == 0) return; // source fully transparent; skip

//   if(sa == 255) {     // source fully opaque; overwrite
//     uint32_t *p = (uint32_t*)dst;
//     uint32_t c = *(uint32_t*)src;
//     while(w--) { *p++ = c; }
//     return;
//   }

//   uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
//   while(w--) {
//     blend_rgba_rgba(dst, r, g, b, a);
//     dst += 4;
//   }
// }

// // blends one rgba source pixel over a horizontal span of destination pixels with alpha mask
// static inline __attribute__((always_inline))
// void mask_span_blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint32_t w, uint8_t *m) {
//   uint8_t r = src[0], g = src[1], b = src[2];
//   uint8_t sa = src[3]; // take copy of original source alpha
//   while(w--) {
//     uint16_t t = *m * sa + 128; // combine source alpha with mask alpha
//     uint8_t a = (t + (t >> 8)) >> 8;
//     blend_rgba_rgba(dst, r, g, b, a);
//     dst += 4;
//     m++;
//   }
// }

/*

  blitting functions

*/

// blends a horizontal run of rgba source pixels onto the corresponding destination pixels
// static inline __attribute__((always_inline))
// void _span_blit_rgba_rgba(uint8_t *dst, color_t c, int w, uint8_t a) {
//   while(w--) {
//     uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
//     *dst = blend_func_over(*dst, r, g, b, a);
//     //_blend_rgba_rgba(dst, src, a);
//     dst += 4;
//     src += 4;
//   }
// }

// // blends a horizontal run of rgba source pixels from a palette onto the corresponding destination pixels
// static inline __attribute__((always_inline))
// void _span_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, int w, uint8_t a) {
//   while(w--) {
//     uint8_t *col = &pal[*src << 2];
//     uint8_t r = col[0], g = col[1], b = col[2], a = col[3];
//     *dst = blend_rgba_rgba(*dst, r, g, b, a);
//     //_blend_rgba_rgba(dst, &pal[*src << 2], a);
//     dst += 4;
//     src++;
//   }
// }

// static inline __attribute__((always_inline))
// void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, int x, int step, int w, uint8_t a) {
//   while(w--) {
//     uint8_t *osrc = src + ((x >> 16) << 2);
//     uint8_t r = osrc[0], g = osrc[1], b = osrc[2], a = osrc[3];
//     blend_rgba_rgba(dst, r, g, b, a);
//     dst += 4;
//     x += step;
//   }
// }

// static inline __attribute__((always_inline))
// void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, int x, int step, int w, uint8_t a) {
//   while(w--) {
//     uint8_t *osrc = src + (x >> 16);
//     uint8_t *col = &pal[*osrc << 2];
//     uint8_t r = col[0], g = col[1], b = col[2], a = col[3];

//     blend_rgba_rgba(dst, r, g, b, a);
//     dst += 4;
//     x += step;
//   }
// }
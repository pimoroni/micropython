#pragma once

#include <stdint.h>


// TODO: this function probably doesn't belong here...?
static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  return __builtin_bswap32((r << 24) | (g << 16) | (b << 8) | a);
}

inline uint32_t hsv(int h, int s, int v, uint8_t a = 255) {
  int region = h / 60;
  int remainder = (h - (region * 60)) * 255 / 60;

  int p = (v * (255 - s)) / 255;
  int q = (v * (255 - (s * remainder) / 255)) / 255;
  int t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;

  uint8_t r, g, b;
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default:
      r = v; g = p; b = q; break;
  }
  return rgba(r, g, b, a);
}


  static inline float clamp01(float x) {
      if (x < 0.0f) return 0.0f;
      if (x > 1.0f) return 1.0f;
      return x;
  }


  static inline float srgb_encode(float x) {
      x = clamp01(x);
      if (x <= 0.0031308f) {
          return 12.92f * x;
      } else {
          return 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
      }
  }

inline uint32_t oklch(int l_in, int c_in, int h_in, uint8_t a_in = 255) {
  // Normalise to OKLCH ranges
  float L = (float)l_in / 255.0f;   // 0–1

  float t = (float)c_in / 255.0f;      // 0–1
  // Slightly compress the top end: more resolution at low/mid chroma
  t = 1.0f - (1.0f - t) * (1.0f - t);  // simple quadratic ease-out
  const float OKLCH_MAX_CHROMA = 0.35f;
  float C = t * OKLCH_MAX_CHROMA;

  // Wrap hue and convert to radians
  if (h_in < 0) {
    h_in %= 360;
    if (h_in < 0) h_in += 360;
  }
  h_in %= 360;
  float h = (float)h_in * (float)M_PI / 180.0f;

  // OKLCH → OKLab
  float a = C * cosf(h);
  float b = C * sinf(h);

  // OKLab → LMS (non-linear)
  float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
  float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
  float s_ = L - 0.0894841775f * a - 1.2914855480f * b;

  // Cube to get linear LMS
  l_ = l_ * l_ * l_;
  m_ = m_ * m_ * m_;
  s_ = s_ * s_ * s_;

  // LMS → linear sRGB
  float r_lin =  4.0767416621f * l_ - 3.3077115913f * m_ + 0.2309699292f * s_;
  float g_lin = -1.2684380046f * l_ + 2.6097574011f * m_ - 0.3413193965f * s_;
  float b_lin = -0.0041960863f * l_ - 0.7034186147f * m_ + 1.7076147010f * s_;

  // Linear → sRGB
  float r_srgb = srgb_encode(r_lin);
  float g_srgb = srgb_encode(g_lin);
  float b_srgb = srgb_encode(b_lin);

  // Convert to 0–255
  int ri = (int)(r_srgb * 255.0f + 0.5f);
  int gi = (int)(g_srgb * 255.0f + 0.5f);
  int bi = (int)(b_srgb * 255.0f + 0.5f);

  // Clamp to valid byte range
  if (ri < 0) ri = 0; else if (ri > 255) ri = 255;
  if (gi < 0) gi = 0; else if (gi > 255) gi = 255;
  if (bi < 0) bi = 0; else if (bi > 255) bi = 255;

  return rgba(ri, gi, bi, a_in);
}

static inline __attribute__((always_inline))
uint32_t get_r(const uint32_t *c) {uint8_t *p = (uint8_t*)c; return p[0];}
static inline __attribute__((always_inline))
uint32_t get_g(const uint32_t *c) {uint8_t *p = (uint8_t*)c; return p[1];}
static inline __attribute__((always_inline))
uint32_t get_b(const uint32_t *c) {uint8_t *p = (uint8_t*)c; return p[2];}
static inline __attribute__((always_inline))
uint32_t get_a(const uint32_t *c) {uint8_t *p = (uint8_t*)c; return p[3];}

static inline __attribute__((always_inline))
void set_r(const uint32_t *c, uint8_t r) {uint8_t *p = (uint8_t*)c; p[0] = r;}
static inline __attribute__((always_inline))
void set_g(const uint32_t *c, uint8_t g) {uint8_t *p = (uint8_t*)c; p[1] = g;}
static inline __attribute__((always_inline))
void set_b(const uint32_t *c, uint8_t b) {uint8_t *p = (uint8_t*)c; p[2] = b;}
static inline __attribute__((always_inline))
void set_a(const uint32_t *c, uint8_t a) {uint8_t *p = (uint8_t*)c; p[3] = a;}

// TODO: consider making all images pre-multiplied alpha

// NOTE: previously we had blend paths using the rp2 interpolators but these
// turn out to be slower due to mmio access times, often taking around 40%
// longer to do the same work


// blends a source rgba pixel over a destination rgba pixel
// (~30 cycles per pixel)
static inline __attribute__((always_inline))
void blend_rgba_rgba(uint8_t *dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  int dr = dst[0], dg = dst[1], db = dst[2], da = dst[3];
  dst[0] = dr + ((a * (r - dr)) >> 8);
  dst[1] = dg + ((a * (g - dg)) >> 8);
  dst[2] = db + ((a * (b - db)) >> 8);
  dst[3] = a + ((da * (255 - a)) >> 8);
}

// blends one rgba source pixel over a horizontal span of destination pixels
static inline __attribute__((always_inline))
void span_blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint32_t w) {
  uint8_t sa = src[3];

  if(sa == 0) return; // source fully transparent; skip

  if(sa == 255) {     // source fully opaque; overwrite
    uint32_t *p = (uint32_t*)dst;
    uint32_t c = *(uint32_t*)src;
    while(w--) { *p++ = c; }
    return;
  }

  uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
  while(w--) {
    blend_rgba_rgba(dst, r, g, b, a);
    dst += 4;
  }
}

// blends one rgba source pixel over a horizontal span of destination pixels with alpha mask
static inline __attribute__((always_inline))
void mask_span_blend_rgba_rgba(uint8_t *dst, uint8_t *src, uint32_t w, uint8_t *m) {
  uint8_t r = src[0], g = src[1], b = src[2];
  uint8_t sa = src[3]; // take copy of original source alpha
  while(w--) {
    uint16_t t = *m * sa + 128; // combine source alpha with mask alpha
    uint8_t a = (t + (t >> 8)) >> 8;
    blend_rgba_rgba(dst, r, g, b, a);
    dst += 4;
    m++;
  }
}

/*

  blitting functions

*/

// blends a horizontal run of rgba source pixels onto the corresponding destination pixels
static inline __attribute__((always_inline))
void _span_blit_rgba_rgba(uint8_t *dst, uint8_t *src, int w, uint8_t a) {
  while(w--) {
    uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
    blend_rgba_rgba(dst, r, g, b, a);
    //_blend_rgba_rgba(dst, src, a);
    dst += 4;
    src += 4;
  }
}

// blends a horizontal run of rgba source pixels from a palette onto the corresponding destination pixels
static inline __attribute__((always_inline))
void _span_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, int w, uint8_t a) {
  while(w--) {
    uint8_t *col = &pal[*src << 2];
    uint8_t r = col[0], g = col[1], b = col[2], a = col[3];
    blend_rgba_rgba(dst, r, g, b, a);
    //_blend_rgba_rgba(dst, &pal[*src << 2], a);
    dst += 4;
    src++;
  }
}

static inline __attribute__((always_inline))
void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, int x, int step, int w, uint8_t a) {
  while(w--) {
    uint8_t *osrc = src + ((x >> 16) << 2);
    uint8_t r = osrc[0], g = osrc[1], b = osrc[2], a = osrc[3];
    blend_rgba_rgba(dst, r, g, b, a);
    dst += 4;
    x += step;
  }
}

static inline __attribute__((always_inline))
void _span_scale_blit_rgba_rgba(uint8_t *dst, uint8_t *src, uint8_t *pal, int x, int step, int w, uint8_t a) {
  while(w--) {
    uint8_t *osrc = src + ((x >> 16) << 2);
    uint8_t *col = &pal[*osrc << 2];
    uint8_t r = col[0], g = col[1], b = col[2], a = col[3];

    blend_rgba_rgba(dst, r, g, b, a);
    dst += 4;
    x += step;
  }
}
#include <algorithm>

#include "picovector.hpp"
#include "brush.hpp"
#include "image.hpp"
#include "shape.hpp"
#include "types.hpp"
#include "mat3.hpp"
#include "blend.hpp"

using std::sort;

// memory pool for rasterisation, png decoding, and other memory intensive
// tasks (sized to fit PNGDEC state) - on pico it *must* be 32bit aligned (i
// found out the hard way.)
char __attribute__((aligned(4))) PicoVector_working_buffer[working_buffer_size];

// This will completely break imgui or sokol or something
//because these will be called before the MicroPython heap is initialised.

bool micropython_gc_enabled = false;
/*
void * operator new(std::size_t n)// throw(std::bad_alloc)
{
    //return malloc(n);
    if(micropython_gc_enabled) {
      std::cout << "new: m_tracked_calloc(" << n << ")" << std::endl;
      return m_tracked_calloc(n, 1);
    } else {
      return malloc(n);
    }
}

void operator delete(void * p)// throw()
{
    //std::cout << "free: " << reinterpret_cast<void*>(p) << std::dec << std::endl;
    //free(p);
    if(micropython_gc_enabled) {
      std::cout << "free: m_tracked_free(" << reinterpret_cast<void*>(p) << std::dec << ")" << std::endl;
      m_tracked_free(p);
    } else {
      free(p);
    }
}
*/

//using namespace std;

//#define debug_printf(fmt, ...)
//#define debug_printf(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

namespace picovector {
  struct _edgeinterp {
    point_t s;
    point_t e;
    float step;

    _edgeinterp() {

    }

    _edgeinterp(point_t p1, point_t p2) {
      if(p1.y < p2.y) {
        s = p1; e = p2;
      } else {
        s = p2; e = p1;
      }
      step = (e.x - s.x) / (e.y - s.y);
    }

    void next(float y, float *nodes, int &node_count) {
      if(y < s.y || y >= e.y) return;
      nodes[node_count++] = s.x + ((y - s.y) * step);
    }
  };

  void render(shape_t *shape, image_t *target, mat3_t *transform, brush_t *brush) {
    if(!shape->paths.size()) {return;};

    // determine the intersection between transformed polygon and target image
    rect_t b = shape->bounds();

    // clip the shape bounds to the target bounds
    rect_t cb = b.intersection(target->clip());
    cb.x = floor(cb.x);
    cb.y = floor(cb.y);
    cb.w = ceil(cb.w);
    cb.h = ceil(cb.h);

    //debug_printf("rendering shape %p with %d paths\n", (void*)shape, int(shape->paths.size()));
    //debug_printf("setup interpolators\n");
    // setup interpolators for each edge of the polygon
    //static _edgeinterp edge_interpolators[256];
    auto edge_interpolators = new(PicoVector_working_buffer) _edgeinterp[256];
    int edge_interpolator_count = 0;
    for(path_t &path : shape->paths) {
      point_t last = path.points.back(); // start with last point to close loop
      last = last.transform(transform);
      //debug_printf("- adding path with %d points\n", int(path.points.size()));
      for(point_t next : path.points) {
        next = next.transform(transform);
        // add new edge interpolator
        edge_interpolators[edge_interpolator_count] = _edgeinterp(last, next);
        edge_interpolator_count++;
        last = next;
      }
    }

    // for each scanline we step the interpolators and build the list of
    // intersecting nodes for that scaline
    static float nodes[128]; // up to 128 nodes (64 spans) per scanline
    const size_t SPAN_BUFFER_SIZE = 512;
    //static _rspan spans[SPAN_BUFFER_SIZE];
    static auto spans = new((uint8_t *)PicoVector_working_buffer + (sizeof(_edgeinterp) * 256)) _rspan[SPAN_BUFFER_SIZE];

    //static uint8_t sb[SPAN_BUFFER_SIZE];
    static auto sb = new((uint8_t *)PicoVector_working_buffer + (sizeof(_edgeinterp) * 256) + (sizeof(_edgeinterp) * SPAN_BUFFER_SIZE)) uint8_t[SPAN_BUFFER_SIZE];

    int aa = target->antialias();

    int sy = cb.y;
    int ey = cb.y + cb.h;

    // TODO: we can special case a faster version for no AA here

    int span_count = 0;
    for(float y = sy; y < ey; y++) {
      // clear the span buffer
      memset(sb, 0, cb.w);

      // loop over y sub samples
      for(int yss = 0; yss < aa; yss++) {
        float ysso = (1.0f / float(aa + 1)) * float(yss + 1);

        int node_count = 0;

        for(int i = 0; i < edge_interpolator_count; i++) {
          edge_interpolators[i].next(y + ysso, nodes, node_count);
        }

        // sort the nodes so that neighouring pairs represent render spans
        sort(nodes, nodes + node_count);

        for(int i = 0; i < node_count; i += 2) {
          int x1 = round((nodes[i + 0] - cb.x) * aa);
          int x2 = round((nodes[i + 1] - cb.x) * aa);

          x1 = min(max(0, x1), int(cb.w * aa));
          x2 = min(max(0, x2), int(cb.w * aa));
          uint8_t *psb = sb;
          for(int j = x1; j < x2; j++) {
            psb[j >> (aa >> 1)]++;
          }
        }
      }

      // todo: this could be more efficient if we buffer multiple scanlines at once
      //debug_printf("render_span_buffer\n");
      static uint8_t aa_none[2] = {0, 255};
      static uint8_t aa_x2[5] = {0, 64, 128, 192, 255};
      static uint8_t aa_x4[17] = {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255};

      uint8_t *aa_lut = aa_none;
      aa_lut = aa == 2 ? aa_x2 : aa_lut;
      aa_lut = aa == 4 ? aa_x4 : aa_lut;

      // scale span buffer alpha values
      int c = cb.w;
      uint8_t *psb = sb;
      while(c--) {
        *psb = aa_lut[*psb];
        psb++;
      }

      brush->mask_span_func(brush, cb.x, y, cb.w, sb);
      //brush->render_span_buffer(target, cb.x, y, cb.w, sb);
    }

    bool _debug_points = false;
    if(_debug_points) {
      color_brush_t white(target, rgba(255, 255, 255, 50));
      for(path_t &path : shape->paths) {
        point_t last = path.points.back(); // start with last point to close loop
        last = last.transform(transform);
        //debug_printf("- adding path with %d points\n", int(path.points.size()));
        for(point_t next : path.points) {
          // _rspan span = {.x = next.x .y = next.y, w = 1, o = 255};
          if(next.x >= 0 && next.x < 160 && next.y >= 0 && next.y < 120) {
            white.pixel_func(&white, next.x, next.y);
            //white.render_span(target, next.x, next.y, 1);
          }
        }
      }
    }
  }



}

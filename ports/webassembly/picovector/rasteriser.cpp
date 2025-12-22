#include <algorithm>

#include "types.hpp"
#include "rasteriser.hpp"

namespace picovector {
  struct edge_t {
    fx16_point_t *p1;
    fx16_point_t *p2;
  };

  struct node_t {
    int x;
    int y;
  };

  struct bounds_t {
    int x1;
    int y1;
    int x2;
    int y2;
  };

  // tile buffer
  constexpr int max_tile_width = 64;
  constexpr int max_tile_height = 64;
  constexpr size_t tile_buffer_offset = 0;
  constexpr size_t tile_buffer_size = max_tile_width * max_tile_height;
  uint8_t *tile = (uint8_t*)(PicoVector_working_buffer + tile_buffer_offset);

  // scanline node buffer
  constexpr int max_nodes = 8192;
  constexpr size_t node_buffer_offset = tile_buffer_offset + tile_buffer_size;
  constexpr size_t node_buffer_size = sizeof(node_t) * max_nodes;
  node_t *nodes = (node_t*)(PicoVector_working_buffer + node_buffer_offset);

  // path points buffer size
  constexpr int max_points = 1024;
  constexpr size_t point_buffer_offset = node_buffer_offset + node_buffer_size;
  constexpr size_t point_buffer_size = sizeof(fx16_point_t) * max_points;
  fx16_point_t *points = (fx16_point_t*)(PicoVector_working_buffer + point_buffer_offset);

  // edge buffer size
  constexpr int max_edges = 1024;
  constexpr size_t edge_buffer_offset = point_buffer_offset + point_buffer_size;
  constexpr size_t edge_buffer_size = sizeof(edge_t) * max_edges;
  edge_t *edges = (edge_t*)(PicoVector_working_buffer + edge_buffer_offset);

  // buffer counters
  int node_count = 0;
  int point_count = 0;
  int edge_count = 0;
  fx16_t minx = INT_MAX;
  fx16_t miny = INT_MAX;
  fx16_t maxx = INT_MIN;
  fx16_t maxy = INT_MIN;

  int aa_scale = 1;

  void pvr_reset() {
    node_count = 0;
    point_count = 0;
    edge_count = 0;
    minx = INT_MAX;
    miny = INT_MAX;
    maxx = INT_MIN;
    maxy = INT_MIN;
  }

  // add a new path to the rasteriser with optional transformation matrix
  void pvr_add_path(point_t *p, int count, mat3_t *transform) {
    // transform path points, convert to fixed point, scale for antialiasing
    // and add to points buffer
    for(int i = 0; i < count; i++) {
      point_t t = p->transform(*transform);
      fx16_point_t *fxt = &points[point_count + i];
      fxt->x = f_to_fx16(t.x * aa_scale);
      fxt->y = f_to_fx16(t.y * aa_scale);

      // update overall polygon bounds
      minx = std::min(minx, fxt->x);
      maxx = std::max(maxx, fxt->x);
      miny = std::min(miny, fxt->y);
      maxy = std::max(maxy, fxt->y);

      p++;
    }

    // build edges, swap point indices so that edges always point "down" and
    // discard any horizontal edges
    fx16_point_t *last = &points[count - 1];
    for(int i = 0; i < count; i++) {
      fx16_point_t *next = &points[point_count + i];

      if(last->y == next->y) continue; // skip horizontal edges

      edge_t *edge = &edges[edge_count];
      if(last->y < next->y) {
        edge->p1 = last;
        edge->p2 = next;
      }else{
        edge->p1 = next;
        edge->p2 = last;
      }

      edge_count++;
      last = next;
    }

    point_count += count;
  }

  void pvr_build_nodes(const bounds_t &tb) {
    node_count = 0;

    // printf("- edge count %d\n", edge_count);
    // printf("working buffer size = %d\n", int(sizeof(PicoVector_working_buffer)));
    // printf("buffer size total = %d\n", int(point_buffer_offset + point_buffer_size));

    for(int i = 0; i < edge_count; i++) {
      // printf(" - edge %d\n", i);
      // printf(" - %p -> %p\n", edges[i].p1, edges[i].p2);
      // printf(" - %d, %d -> %d, %d\n", edges[i].p1->x >> 16, edges[i].p1->y >> 16, edges[i].p2->x >> 16, edges[i].p2->y >> 16);

      fx16_point_t *p1 = edges[i].p1;
      fx16_point_t *p2 = edges[i].p2;

      // tile offset in fp16
      fx16_t tx = (tb.x1 << 16);
      fx16_t ty = (tb.y1 << 16);

      // adjust edge points relative to tile offset
      fx16_t sx = p1->x - tx;
      fx16_t sy = p1->y - ty;
      fx16_t ex = p2->x - tx;
      fx16_t ey = p2->y - ty;
      fx16_t x = sx;

      // tile height in fp16
      fx16_t th = (tb.y2 - tb.y1) << 16;

      // if edge not within vertical bounds of tile skip
      if(sy >= th || ey <= 0) continue;

      // calculate x step per scanline
      fx16_t stepx = (ex - sx) / (ey - sy);

      // if edge starts above tile then jump to intersection with top edge
      if(sy < 0) {
        x += stepx * abs(sy);
        sy = 0;
      }

      // if edge ends after tile then clamp to at most tile height
      if(ey > th) {
        ey = th;
      }

      sy >>= 16;
      ey >>= 16;

      //printf(" - interpolate edge %d -> %d\n", sy, ey);
      for(int y = sy; y < ey; y++) {
        nodes[node_count].x = x >> 16;
        nodes[node_count].y = y >> 16;
        node_count++;
        x += stepx;
      }
      //printf(" - node_count %d\n", node_count);
    }

    //printf("- sort nodes\n");
    // sort the nodes by row and by column
    std::sort(nodes, nodes + node_count,
      [](const node_t& a, const node_t& b) {
        if(a.y == b.y) return a.x < b.x;
        return a.y < b.y;
      }
    );
  }

  void pvr_render(image_t *target, rect_t clip, brush_t *brush) {
    // floored and ceiled bounds of the shape
    bounds_t sb;
    sb.x1 = minx >> 16;
    sb.y1 = miny >> 16;
    sb.x2 = (maxx >> 16) + 1;
    sb.y2 = (maxy >> 16) + 1;

    // clip render bounds to clip rectangle
    sb.x1 = std::max(int(floor(clip.x)), sb.x1);
    sb.y1 = std::max(int(floor(clip.y)), sb.y1);
    sb.x2 = std::min(int(ceil(clip.x + clip.w)), sb.x2);
    sb.y2 = std::min(int(ceil(clip.h + clip.h)), sb.y2);

    // get tile width for aa level
    int tw = max_tile_width;
    int th = max_tile_height;

    for(int y = sb.y1; y < sb.y2; y += th) {
      for(int x = sb.x1; x < sb.x2; x += tw) {
        // calculate tile bounds and clamp to shape bounds if needed
        bounds_t tb;
        tb.x1 = x;
        tb.y1 = y;
        tb.x2 = std::min(tb.x1 + tw, sb.x2);
        tb.y2 = std::min(tb.y1 + th, sb.y2);

        //printf("render tile %d, %d\n", x, y);

        // build the nodes for this tile
        pvr_build_nodes(tb);

        for(int i = 0; i < node_count; i += 2) {
          int nsx = nodes[node_count].x;
          int nex = nodes[node_count + 1].x;

          if(nsx == nex) continue;

          int ny = nodes[node_count].y;
          int to = nsx + (ny * tw);
          uint8_t *p = tile + to;

          brush->span_func(brush, nsx, ny, nex - nsx);
//          brush->render_span(target, nsx, ny, nex - nsx);


        }

      }
    }

    miny = INT_MAX;
    maxx = INT_MIN;
    maxy = INT_MIN;
  }
}

/*

};

fx16_t minx, miny, maxx, maxy; // bounds for polygon rendering

int max_tile_height = 32; // maximum number of scanlines per tile
int max_ei = 256; // buffer for up to 256 edge interpolators
int max_nodes = 64; // maximum nodes per scanline

size_t ei_buffer_size = sizeof(ei) * max_ei;
int ei_count = 0; // number of active edge interpolators
ei *eis = (ei*)(PicoVector_working_buffer); // edge interpolator buffer

int node_count = 0;
fx16_t *nodes = (fx16_t*)(PicoVector_working_buffer + ei_buffer_size);



namespace picovector {

  void pr_reset() {
    // reset overall bounds
    minx = INT_MAX;
    miny = INT_MAX;
    maxx = INT_MIN;
    maxy = INT_MIN;

    // reset interpolator count
    ei_count = 0;
    node_count = 0;
  }

  // create an edge interpolator
  void pr_add_edge(point_t p1, point_t p2) {
    if(ei_count < max_ei) {
      eis[ei_count] = ei(p1, p2);
      ei_count++;
    }
  }

  void pr_render() {

  }

  // void render(shape_t *shape, mat3_t *transform, image_t *target, brush_t *brush) {
  //   if(!shape->paths.size()) {return;};

  //   // will contain transformed shape bounds
  //   float minx = FLT_MAX, miny = FLT_MAX, maxx = FLT_MIN, maxy = FLT_MIN;

  //   // setup edge interpolators
  //   int edge_interpolator_count = 0;
  //   for(auto &path : shape->paths) {
  //     auto last = path.points.back(); // start with last point to close loop
  //     last = last.transform(transform);

  //     for(auto next : path.points) {
  //       next = next.transform(transform);

  //       // update transformed bounds
  //       minx = min(minx, next.x);
  //       miny = min(miny, next.y);
  //       maxx = max(maxx, next.x);
  //       maxy = max(maxy, next.y);

  //       // add new edge interpolator
  //       edge_interpolators[edge_interpolator_count] = _edgeinterp(last, next);
  //       edge_interpolator_count++;
  //       last = next;
  //     }
  //   }

  //   rect_t b = shape->bounds();
  //   b.
  //   .transform(transform);

  //   int tile_buffer_size = 8096;
  //   //
  //   64*64
  //   uint8_t *tile_buffer = (uint8_t*)&PicoVector_working_buffer[0];
  // }
}*/
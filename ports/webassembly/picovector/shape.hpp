#pragma once

#include <vector>
#include "picovector.hpp"
#include "mat3.hpp"
#include "types.hpp"

namespace picovector {

  class path_t {
  public:
    std::vector<point_t, PV_STD_ALLOCATOR<point_t>> points;

    path_t(int point_count = 0);
    void add_point(const point_t &point);
    void add_point(float x, float y);
    void edge_points(int edge, point_t &s, point_t &e);
    void offset_edge(point_t &s, point_t &e, float offset);
    void stroke(float offset);
    void inflate(float offset);
  };

  class shape_t {
  public:
    std::vector<path_t, PV_STD_ALLOCATOR<path_t>> paths;
    mat3_t transform;
    brush_t *_brush = nullptr;

    shape_t(int path_count = 0);
    ~shape_t() {
      //debug_printf("shape destructed\n");
    }
    void add_path(path_t path);
    rect_t bounds();
    /*void draw(image &img); // methods should be on image perhaps? with style/brush and transform passed in?*/
    void stroke(float thickness);
    void brush(brush_t *brush);
  };

}
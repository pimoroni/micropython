#include <float.h>
#include "shape.hpp"

using std::vector;

namespace picovector {


  void offset_line_segment(vec2_t &s, vec2_t &e, float offset) {
    // calculate normal of edge
    float nx = -(e.y - s.y);
    float ny = e.x - s.x;
    float l = sqrt(nx * nx + ny * ny);
    nx /= l;
    ny /= l;

    // scale normal to requested offset
    float ox = nx * offset;
    float oy = ny * offset;

    // offset supplied edge vec2s
    s.x += ox;
    s.y += oy;
    e.x += ox;
    e.y += oy;
  }

  bool intersection(vec2_t p1, vec2_t p2, vec2_t p3, vec2_t p4, vec2_t &i) {
    float a1 = p2.y - p1.y;
    float b1 = p1.x - p2.x;
    float c1 = a1 * p1.x + b1 * p1.y;

    float a2 = p4.y - p3.y;
    float b2 = p3.x - p4.x;
    float c2 = a2 * p3.x + b2 * p3.y;

    float determinant = a1 * b2 - a2 * b1;

    if(determinant == 0) {
      return false; // lines parallel or coincident
    }

    i.x = (b2 * c1 - b1 * c2) / determinant;
    i.y = (a1 * c2 - a2 * c1) / determinant;
    return true;
  }



  shape_t::shape_t(int path_count) {
    //debug_printf("shape constructed\n");
    paths.reserve(path_count);
  }

  void shape_t::add_path(path_t path) {
    paths.push_back(path);
  }

  rect_t shape_t::bounds() {
    float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;
    for(const path_t &path : paths) {
      for(vec2_t vec2 : path.points) {
        vec2 = vec2.transform(&transform);
        minx = min(minx, vec2.x);
        miny = min(miny, vec2.y);
        maxx = max(maxx, vec2.x);
        maxy = max(maxy, vec2.y);
      }
    }
    return rect_t(minx, miny, ceil(maxx) - minx, ceil(maxy) - miny);
  }

  // these should be methods on image maybe?
  // void shape::draw(image &img) {
  //   if(style) {
  //     render(*this, img, style);
  //   }
  // }

  void shape_t::brush(brush_t *brush) {
    this->_brush = brush;
  }

  void shape_t::stroke(float thickness) {
    for(int i = 0; i < (int)this->paths.size(); i++) {
      this->paths[i].stroke(thickness);
    }
  }



  path_t::path_t(int vec2_count) {
    points.reserve(vec2_count);
  }

  void path_t::add_point(const vec2_t &vec2) {
    points.push_back(vec2);
  }

  void path_t::add_point(float x, float y) {
    points.push_back(vec2_t(x, y));
  }

  void path_t::edge_points(int edge, vec2_t &s, vec2_t &e) {
    // return the two vec2s that make up an edge
    s = edge == -1 ? points.back() : points[edge];
    e = edge == (int)points.size() - 1 ? points.front() : points[edge + 1];
  }

  void path_t::stroke(float offset) {
    int c = points.size();
    vector<vec2_t, PV_STD_ALLOCATOR<vec2_t>> new_points(c);

    if(c == 2) {
        vec2_t p1, p2; // edge 1 start and end
        edge_points(0, p1, p2);
        offset_line_segment(p1, p2, offset);
        points.push_back(p2);
        points.push_back(p1);
    }else{
      for(int i = 0; i < c; i++) {
        vec2_t p1, p2; // edge 1 start and end
        edge_points(i - 1, p1, p2);
        offset_line_segment(p1, p2, offset);

        vec2_t p3, p4; // edge 2 start and end
        edge_points(i, p3, p4);
        offset_line_segment(p3, p4, offset);

        // find intersection of the edges
        vec2_t pi;
        bool ok = intersection(p1, p2, p3, p4, pi);
        new_points[i] = pi;
      }

      points.push_back(points.front());
      points.insert(points.end(), new_points.begin(), new_points.end());
      points.push_back(new_points.front());
    }
  }

  void path_t::inflate(float offset) {
    vector<vec2_t, PV_STD_ALLOCATOR<vec2_t>> new_points(points.size());

    int edge_count = points.size();
    for(int i = 0; i < edge_count; i++) {
      vec2_t p1, p2; // edge 1 start and end
      edge_points(i, p1, p2);
      offset_line_segment(p1, p2, offset);

      vec2_t p3, p4; // edge 2 start and end
      edge_points(i + 1, p3, p4);
      offset_line_segment(p3, p4, offset);

      // find intersection of the edges
      vec2_t pi;
      bool ok = intersection(p1, p2, p3, p4, pi);
      new_points[i] = pi;
    }

    points = new_points;
  }

}

#pragma once

#include "shape.hpp"

namespace picovector {

  shape_t* regular_polygon(float x, float y, float sides, float radius);
  shape_t* circle(float x, float y, float radius);
  shape_t* rectangle(float x, float y, float w, float h);
  shape_t* rounded_rectangle(float x, float y, float w, float h, float r1, float r2, float r3, float r4);
  shape_t* squircle(float x, float y, float size, float n=4.0f);
  shape_t* arc(float x, float y, float from, float to, float inner, float outer);
  shape_t* pie(float x, float y, float from, float to, float radius);
  shape_t* star(float x, float y, int spikes, float outer_radius, float inner_radius);
  shape_t* line(float x1, float y1, float x2, float y2, float w);

};

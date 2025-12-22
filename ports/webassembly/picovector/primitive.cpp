#include "picovector.hpp"
#include "primitive.hpp"

namespace picovector {

  shape_t* regular_polygon(float x, float y, float sides, float radius) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);
    path_t poly(sides);
    for(int i = 0; i < sides; i++) {
      float theta = ((M_PI * 2.0f) / (float)sides) * (float)i;
      poly.add_point(sin(theta) * radius + x, cos(theta) * radius + y);
    }
    result->add_path(poly);
    return result;
  }

  shape_t* circle(float x, float y, float radius) {
    int sides = 32;
    return regular_polygon(x, y, sides, radius);
  }

  shape_t* rectangle(float x, float y, float w, float h) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);
    path_t poly(4);
    poly.add_point(x, y);
    poly.add_point(x + w, y);
    poly.add_point(x + w, y + h);
    poly.add_point(x, y + h);
    result->add_path(poly);
    return result;
  }

  void _build_rounded_rectangle_corner(path_t *path, float x, float y, float r, int q) {
    float quality = 5; // higher the number, lower the quality - selected by experiment
    int steps = ceil(r / quality) + 1;
    float delta = -(M_PI / 2) / float(steps);
    float theta = (M_PI / 2) * q; // select start theta for this quadrant
    for(int i = 0; i <= steps; i++) {
      float xo = sin(theta) * r, yo = cos(theta) * r;
      path->add_point((point_t){x + xo, y + yo});
      theta += delta;
    }
  }

  shape_t* rounded_rectangle(float x, float y, float w, float h, float r1, float r2, float r3, float r4) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);
    path_t poly(4);

    // render corners (either hard if radius == 0 or calculate rounded corner points)
    r1 == 0 ? poly.add_point((point_t){x    , y    }) : _build_rounded_rectangle_corner(&poly, x + 0 + r1, y + 0 + r1, r1, 3);
    r2 == 0 ? poly.add_point((point_t){x + w, y    }) : _build_rounded_rectangle_corner(&poly, x + w - r2, y + 0 + r2, r2, 2);
    r3 == 0 ? poly.add_point((point_t){x + w, y + h}) : _build_rounded_rectangle_corner(&poly, x + w - r3, y + h - r3, r3, 1);
    r4 == 0 ? poly.add_point((point_t){x    , y + h}) : _build_rounded_rectangle_corner(&poly, x + 0 + r4, y + h - r4, r4, 0);

    result->add_path(poly);
    return result;
  }


    // static shape rounded_rectangle(float x1, float y1, float x2, float y2, float r1, float r2, float r3, float r4, float stroke=0.0f) {
    // }

  shape_t* squircle(float x, float y, float size, float n) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);

    //shape *result = new shape(1);
    constexpr int points = 32;
    path_t poly(points);
    for(int i = 0; i < points; i++) {
        float t = 2 * M_PI * (points - i) / points;
        float ct = cos(t);
        float st = sin(t);

        poly.add_point(
          x + copysign(pow(abs(ct), 2.0 / n), ct) * size,
          y + copysign(pow(abs(st), 2.0 / n), st) * size
        );
    }
    result->add_path(poly);
    return result;
  }

  shape_t* arc(float x, float y, float from, float to, float inner, float outer) {
   shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);

    from = fmod(from, 360.0f) - 90.0f;
    to = fmod(to, 360.0f) - 90.0f;
    float delta = fabs(to - from);
    int steps = (int)(32.0f * (delta / 360.0f));
    from *= (M_PI / 180.0f);
    to *= (M_PI / 180.0f);

    path_t outline(steps + 1); // TODO: is this right?

    float astep = (to - from) / (float)steps;
    float a = from;

    for(int i = 0; i <= steps; i++) {
      outline.add_point(cos(a) * outer + x, sin(a) * outer + y);
      a += astep;
    }

    a -= astep;
    for(int i = 0; i <= steps; i++) {
      outline.add_point(cos(a) * inner + x, sin(a) * inner + y);
      a -= astep;
    }

    //outline.add_point(x, y); // + 1 point?

    result->add_path(outline);

    return result;
  }

  shape_t* pie(float x, float y, float from, float to, float radius) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);

    from = fmod(from, 360.0f) - 90.0f;
    to = fmod(to, 360.0f) - 90.0f;
    float delta = fabs(to - from);
    int steps = (int)(32.0f * (delta / 360.0f));
    from *= (M_PI / 180.0f);
    to *= (M_PI / 180.0f);

    path_t outline(steps + 1); // TODO: is this right?

    float astep = (to - from) / (float)steps;
    float a = from;

    for(int i = 0; i <= steps; i++) {
      outline.add_point(cos(a) * radius + x, sin(a) * radius + y);
      a += astep;
    }

    outline.add_point(x, y); // + 1 point?

    result->add_path(outline);

    return result;
  }


  shape_t* star(float x, float y, int spikes, float outer_radius, float inner_radius) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);
    path_t poly(spikes * 2);
    for(int i = 0; i < spikes * 2; i++) {
      float step = ((M_PI * 2) / (float)(spikes * 2)) * (float)i;
      float r = i % 2 == 0 ? outer_radius : inner_radius;
      poly.add_point(sin(step) * r + x, cos(step) * r + y);
    }
    result->add_path(poly);
    return result;
  }

  shape_t* line(float x1, float y1, float x2, float y2, float w) {
    shape_t *result = new(PV_MALLOC(sizeof(shape_t))) shape_t(1);
    path_t poly(4);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float m = sqrt(dx * dx + dy * dy);
    dx /= m;
    dy /= m;
    float hw = w / 2.0f;

    poly.add_point(x1 + (dy * hw), y1 - (dx * hw));
    poly.add_point(x2 + (dy * hw), y2 - (dx * hw));
    poly.add_point(x2 - (dy * hw), y2 + (dx * hw));
    poly.add_point(x1 - (dy * hw), y1 + (dx * hw));
    result->add_path(poly);

    return result;
  }
}
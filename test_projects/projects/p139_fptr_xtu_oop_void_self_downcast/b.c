#include "shape.h"

static double circle_area(void *self) {
  struct circle *c = self;
  return 3.14159 * c->r * c->r;
}

static const struct shape_vt circle_vt = { circle_area };

void circle_init(struct circle *c, double r) {
  c->base.vt = &circle_vt;
  c->r = r;
}

// GATE fp-vtable-oop
// C-style polymorphism: objects carry a pointer to a constant ops table.
// The call is reached through a *pointer* base (s->ops->area), so the
// target set must come from a type/field-keyed table, not from one named
// struct instance.
//
// Probe subjects:
//   shape_area.$0 (s)      -- call s->ops->area(s) resolves to
//                             {circle_area, rect_area}
//   circle_area.$0 / rect_area.$0 (self : void *) -- concrete downcasts
//   shape_area.$0 must carry the parallel downcast {struct circle *,
//   struct rect *} imported from the resolved targets
struct shape;
struct shape_ops {
  double (*area)(const void *self);
  void (*destroy)(void *self);
};
struct shape {
  const struct shape_ops *ops;
};
struct circle {
  struct shape base;
  double r;
};
struct rect {
  struct shape base;
  double w, h;
};

static double circle_area(const void *self) {
  const struct circle *c = (const struct circle *)self;
  return 3.14159 * c->r * c->r;
}
static void circle_destroy(void *self) { (void)self; }

static double rect_area(const void *self) {
  const struct rect *r = (const struct rect *)self;
  return r->w * r->h;
}
static void rect_destroy(void *self) { (void)self; }

static const struct shape_ops circle_ops = {circle_area, circle_destroy};
static const struct shape_ops rect_ops = {.area = rect_area,
                                          .destroy = rect_destroy};

void circle_init(struct circle *c, double r) {
  c->base.ops = &circle_ops;
  c->r = r;
}
void rect_init(struct rect *r, double w, double h) {
  r->base.ops = &rect_ops;
  r->w = w;
  r->h = h;
}

double shape_area(const struct shape *s) {
  return s->ops->area(s);
}

double demo(void) {
  struct circle c;
  struct rect r;
  circle_init(&c, 1.0);
  rect_init(&r, 2.0, 3.0);
  return shape_area(&c.base) + shape_area(&r.base);
}

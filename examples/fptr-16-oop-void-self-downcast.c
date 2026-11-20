// fptr-16-oop-void-self-downcast.c
//
// Fact F16: object-oriented C with a base struct embedded as first member and
// methods that take `void *self` and downcast to the derived type.  The type
// of `self` at the dispatch site (shape_area) is only knowable from the method
// targets of the vtable slot.
//
// Real world: icecast format plugins keep per-format state in a void * member
// and downcast it inside each method:
//   void *_state;                                  (src/format.h:57)
//   ogg_state_t *ogg_info = source->format->_state; (src/format_ogg.c:279)
// with methods installed per format (src/format_mp3.c:87).
//
// @fptr-fact F16 oop-void-self-downcast
// @subject   shape_area::self
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=struct_circle* resolution=exact@L1
struct shape_vt { double (*area)(void *self); };
struct shape { const struct shape_vt *vt; };
struct circle { struct shape base; double r; };

static double circle_area(void *self) {
  struct circle *c = self;
  return 3.14159 * c->r * c->r;
}

static const struct shape_vt circle_vt = { circle_area };

void circle_init(struct circle *c, double r) {
  c->base.vt = &circle_vt;
  c->r = r;
}

double shape_area(struct shape *s) {
  void *self = s;
  return s->vt->area(self);
}

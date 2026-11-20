/*
 * Fact X08 (cross-TU F16): the generic dispatcher shape_area() is in a.c; the
 * derived class (circle), its method and vtable are in b.c.  The type of
 * `self` at the dispatch site comes from b.c's method summary.
 *
 * Real world: icecast format plugins (format.h:57, format_ogg.c:279,
 * format_mp3.c:87).
 *
 * @fptr-fact X08 xtu-oop-void-self-downcast
 * @subject   shape_area::self
 * @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=struct_circle* resolution=exact@L1
 */
#include "shape.h"

double shape_area(struct shape *s) {
  void *self = s;
  return s->vt->area(self);
}

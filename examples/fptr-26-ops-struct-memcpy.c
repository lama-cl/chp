// fptr-26-ops-struct-memcpy.c
//
// Fact F26: an ops struct copied with memcpy rather than `=`.  Struct copy
// assignment and initializers propagate field targets today (p40/p41); a
// memcpy of the whole struct must do the same.
//
// Real world: drivers that clone a default ops table before overriding a few
// slots.
//
// @fptr-fact F26 ops-struct-memcpy
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
#include <string.h>

typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

struct ops { handler_t read; };

int run(void *ctx) {
  struct ops defaults = { use_int };
  struct ops mine;
  memcpy(&mine, &defaults, sizeof mine);
  return mine.read(ctx);
}

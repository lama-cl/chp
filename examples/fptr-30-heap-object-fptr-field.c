// fptr-30-heap-object-fptr-field.c
//
// Fact F30: a heap-allocated object receives its function-pointer field in a
// constructor and the method is invoked through a different pointer in a
// different function.  Same struct-type slot requirement as F15, but without a
// separate const ops table (method pointer stored directly in the object).
//
// Real world: LMDB stores comparators in its db records:
//   (env->me_dbxs + 0)->md_cmp = & mdb_cmp_long;
// (test_projects/bench-genc2rust/lmdb/main.c:3938).
//
// @fptr-fact F30 heap-object-fptr-field
// @subject   obj_call::o.data
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
#include <stdlib.h>

typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

struct obj { handler_t fn; void *data; };

struct obj *obj_new(void *data) {
  struct obj *o = malloc(sizeof *o);
  o->fn = use_int;
  o->data = data;
  return o;
}

int obj_call(struct obj *o) { return o->fn(o->data); }

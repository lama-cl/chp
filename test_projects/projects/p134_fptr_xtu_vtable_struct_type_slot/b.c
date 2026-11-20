#include "ops.h"

static int use_int(void *p) { int *q = p; return *q; }

static const struct ops int_ops = { use_int };

void obj_init(struct obj *o, void *data) {
  o->vt = &int_ops;
  o->data = data;
}

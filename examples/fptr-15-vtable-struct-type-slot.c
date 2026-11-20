// fptr-15-vtable-struct-type-slot.c
//
// Fact F15: vtable-like dispatch.  A const ops instance is installed into an
// object in one function and invoked through o->vt->read(o->data) in another.
// Field targets are keyed by syntactic base today (o in obj_init vs o in
// obj_read are unrelated), so obj_read cannot resolve the call.
//
// Required: L1 struct-type slot facts keyed by record type + field path:
//   struct ops::read may hold {use_int}.
//
// Real world: libarchive's struct archive_vtable
// (test_projects/bench-3c/libarchive-3.4.3/libarchive/archive_private.h:63),
// called as a->vtable->archive_filter_code(a, n) (archive_virtual.c:36).
//
// @fptr-fact F15 vtable-struct-type-slot
// @subject   obj_read::o.data
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

struct ops { handler_t read; };
struct obj { const struct ops *vt; void *data; };

static const struct ops int_ops = { use_int };

void obj_init(struct obj *o, void *data) {
  o->vt = &int_ops;
  o->data = data;
}

int obj_read(struct obj *o) { return o->vt->read(o->data); }

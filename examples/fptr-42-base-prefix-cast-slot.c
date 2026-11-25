// fptr-42-base-prefix-cast-slot.c
//
// Fact F42 (soundness guard for D7'): C-style inheritance through a common
// initial member.  Two "derived" records install different targets in their
// first member; a dispatcher reads the slot through a cast to a layout-
// compatible "base" record.  A record-type-keyed slot (record:struct base.fn)
// has NO stores, so a naive L1 lookup would return an empty set or nothing.
// Required: the base slot is the union of every record slot it may alias
// through a pointer cast (sound set) -- never "no targets".  If that cannot be
// computed, the acceptable fallback is resolution=unresolved (not exact).
//
// Real world: CPython-style object headers / GObject class structs
// (first-member embedding with casts between record pointer types).
//
// @fptr-fact F42 base-prefix-cast-slot
// @subject   dispatch::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L1
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
struct base { handler_t fn; };
struct derived_i { handler_t fn; int v; };
struct derived_d { handler_t fn; double v; };
void init_i(struct derived_i *o) { o->fn = use_int; }
void init_d(struct derived_d *o) { o->fn = use_dbl; }
int dispatch(void *obj, void *ctx) {
  struct base *b = obj;
  return b->fn(ctx);
}

// fptr-54-object-correlated-table.c
//
// Fact F54 (D30; round 4): table dispatch selected by the object itself,
// tbl[o->kind](o->data), and each kind is read through its own handler.
// Different views per target, but the selector derives from the same object
// as the argument: safe polymorphism, dispatch=object-correlated.
//
// Today (measured): Opaque, unresolved (no table resolution).
//
// @fptr-fact F54 object-correlated-table
// @subject   call::o.data
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=- resolution=unresolved@- sigmismatch=false
// @expected  category=GuardedActiveReinterpretation types=double*,int* resolution=set@L1 dispatch=object-correlated
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
struct obj { int kind; void *data; };
static const handler_t tbl[2] = { use_int, use_dbl };
int call(struct obj *o) { return tbl[o->kind & 1](o->data); }

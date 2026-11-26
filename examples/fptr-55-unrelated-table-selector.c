// fptr-55-unrelated-table-selector.c
//
// Fact F55 (D30; round 4): the same table, but the selector is a global
// counter unrelated to the object: one object can be read as int* and as
// double* by successive calls.  dispatch=unrelated.
//
// Today (measured): Opaque, unresolved (no table resolution).
//
// @fptr-fact F55 unrelated-table-selector
// @subject   call::o.data
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=- resolution=unresolved@- sigmismatch=false
// @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L1 dispatch=unrelated
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
struct obj { int kind; void *data; };
static const handler_t tbl[2] = { use_int, use_dbl };
static int tick;
int call(struct obj *o) { return tbl[tick++ & 1](o->data); }

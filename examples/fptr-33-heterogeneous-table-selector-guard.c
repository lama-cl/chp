// fptr-33-heterogeneous-table-selector-guard.c
//
// Fact F33 (policy): a dispatch table whose entries view the context as
// different types.  Each table entry is selected by the index value, so the
// alternatives are *guarded by the selector* (table[kind] == use_int), not
// unguarded parallel casts.  Decision D8: L1 table/slot sets carry a synthetic
// selection guard, so this lands in GuardedActiveReinterpretation.
//
// Real world: tagged-union handlers indexed by a type tag (visitor / printer
// tables keyed by an enum); a common C idiom, no bench instance pinned.
//
// @fptr-fact F33 heterogeneous-table-selector-guard
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=GuardedActiveReinterpretation types=double*,int* resolution=set@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

static const handler_t by_kind[] = { use_int, use_dbl };

int run(unsigned kind, void *ctx) { return by_kind[kind & 1](ctx); }

// fptr-34-global-defined-after-use.c
//
// Fact F34 (order dependence): identical to F03 except that the global's
// initialized definition appears *after* the function that calls through it.
// Today the call is unresolved because L0 state only contains file-scope
// initializers already traversed.  Resolution must not depend on declaration
// order: L1 global facts are collected for the whole TU before solving.
//
// @fptr-fact F34 global-defined-after-use
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

extern handler_t g_handler;

int run(void *ctx) { return g_handler(ctx); }

handler_t g_handler = use_int;

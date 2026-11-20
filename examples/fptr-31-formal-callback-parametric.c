// fptr-31-formal-callback-parametric.c
//
// Fact F31 (control): a callback formal parameter with no local information is
// *parametric*, not unresolved: apply's summary keeps $0.$0 and callers
// specialize it.  The new facet must say `parametric`, so that aggregate
// coverage numbers do not count generic helpers as analysis failures.
//
// @fptr-fact F31 formal-callback-parametric
// @subject   apply::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=FormalCallbackParameter behavior=CallbackDependentPointer types=-
// @expected  category=OpaqueTransportOrViewOnly callback=FormalCallbackParameter resolution=parametric@L0
typedef int (*handler_t)(void *);

int apply(handler_t fn, void *ctx) { return fn(ctx); }

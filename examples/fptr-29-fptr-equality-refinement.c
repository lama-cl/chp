// fptr-29-fptr-equality-refinement.c
//
// Fact F29: comparing a function pointer against a known function narrows its
// target inside the guarded region.  cb is a formal callback (unresolved in the
// summary), but under `cb == use_int` the call is exactly use_int.
//
// Real world: "is this the default handler?" checks, e.g. signal(SIG,
// SIG_DFL) comparisons and `if (alloc == default_alloc)` fast paths.
//
// @fptr-fact F29 fptr-equality-refinement
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=FormalCallbackParameter behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

int run(handler_t cb, void *ctx) {
  if (cb == use_int)
    return cb(ctx);
  return 0;
}

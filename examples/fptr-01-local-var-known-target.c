// fptr-01-local-var-known-target.c
//
// Fact F01 (control, supported today): a local function-pointer variable is
// initialized with a function designator in the same function and then called.
// L0 flow-sensitive tracking (functionPointerTargets) resolves cb -> use_int,
// so run::ctx inherits use_int's int* evidence.
//
// Required after the fptr extension: unchanged classification, plus the new
// indirectCallResolution facet reporting an exact target found by layer L0.
//
// @fptr-fact F01 local-var-known-target
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

int run(void *ctx) {
  handler_t cb = use_int;
  return cb(ctx);
}

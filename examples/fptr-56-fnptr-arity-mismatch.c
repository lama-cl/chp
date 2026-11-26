// fptr-56-fnptr-arity-mismatch.c
//
// Fact F56 (fn-type casts, D29; round 4): a target taking two parameters is
// called through a one-parameter type.  Incompatible signature: label
// fnptr-incompatible, signatureMismatch=true, ubkind=signature-incompatible
// (C17 6.5.2.2p9).
//
// Today (measured): exact int*, sigmismatch=false — the incompatible call is not flagged.
//
// @fptr-fact F56 fnptr-arity-mismatch
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  resolution=exact@L0 sigmismatch=true ubkind=signature-incompatible
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
static int use_two(void *p, int n) { int *q = p; return *q + n; }
int run(void *ctx) {
  handler_t cb = (handler_t)use_two;
  return cb(ctx);
}

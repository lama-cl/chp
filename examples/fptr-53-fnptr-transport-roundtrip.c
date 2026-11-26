// fptr-53-fnptr-transport-roundtrip.c
//
// Fact F53 (fn-type casts, D29; round 4): a function pointer stored as a
// generic `void (*)(void)` (the G_CALLBACK idiom) and cast back to its own
// type before the call.  The cast is transport only: label fnptr-transport,
// no signatureMismatch.
//
// Today (measured): correct (exact int*, no flag); control, claimed by M1.
//
// @fptr-fact F53 fnptr-transport-roundtrip
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0 sigmismatch=false
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
typedef void (*generic_fn)(void);
static generic_fn stored;
int run(void *ctx) {
  stored = (generic_fn)use_int;
  handler_t cb = (handler_t)stored;
  return cb(ctx);
}

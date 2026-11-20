// fptr-10-loop-backedge-unsound.c
//
// Fact F10 (SOUNDNESS BUG today): a function pointer reassigned inside a loop
// after the call.  The second iteration calls use_dbl, but the single
// syntactic pass only sees use_int at the call, so run::ctx is reported as a
// monomorphic int* specialization.
//
// Required: a loop widening pass over fptr target state (state at loop head =
// entry ∪ back-edge), producing the unguarded set {use_int, use_dbl}.
//
// @fptr-fact F10 loop-backedge-unsound
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

int run(int n, void *ctx) {
  handler_t cb = use_int;
  int acc = 0;
  for (int i = 0; i < n; i++) {
    acc += cb(ctx);
    cb = use_dbl;
  }
  return acc;
}

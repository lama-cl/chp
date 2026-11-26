// fptr-50-goto-retry-loop.c
//
// Fact F50 (CFG, goto + loop; round 4): a backward goto forms a loop.  The
// same object x is passed to use_int on the first attempt and to use_dbl on
// a retry: a real reinterpretation of one object over time (D30 unrelated).
//
// Today (measured): exact int* — UNSOUND, the retry path with use_dbl is lost (no back edge).
//
// @fptr-fact F50 goto-retry-loop
// @subject   run::x
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0 dispatch=unrelated
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int r, void *x) {
  handler_t cb = use_int;
  int rc;
again:
  rc = cb(x);
  if (r-- > 0) {
    cb = use_dbl;
    goto again;
  }
  return rc;
}

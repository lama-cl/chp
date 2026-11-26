// fptr-47-switch-fallthrough.c
//
// Fact F47 (CFG, conditions; round 4): case 0 assigns use_dbl and falls
// through into case 1, which calls cb(x).  At the call, cb is use_dbl (from
// case 0) or use_int (entered at case 1).  The selector k is not derived from
// x, so for D30 the dispatch is unrelated.
//
// Today (measured): exact double* — UNSOUND, entry at case 1 with use_int is lost (last store wins).
//
// @fptr-fact F47 switch-fallthrough
// @subject   run::x
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  types=double*,int* resolution=set@L0 dispatch=unrelated
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int k, void *x) {
  handler_t cb = use_int;
  switch (k) {
  case 0:
    cb = use_dbl;
    /* fallthrough */
  case 1:
    return cb(x);
  default:
    return 0;
  }
}

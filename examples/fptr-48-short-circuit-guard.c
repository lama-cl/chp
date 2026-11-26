// fptr-48-short-circuit-guard.c
//
// Fact F48 (CFG, conditions; round 4): `if (a && b)` is two CFG branches.
// The guard on use_dbl must be the conjunction (a) && (b); today && and ||
// produce no guard structure (07 §2).
//
// Today (measured): set double*,int* (right set by accident; no (a) && (b) guard).
//
// @fptr-fact F48 short-circuit-guard
// @subject   run::x
// @baseline  category=UnguardedMultiTargetActiveReinterpretation callback=ResolvedCallback behavior=MultiTypeRead types=double*,int* resolution=set@L0 sigmismatch=false
// @expected  types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int a, int b, void *x) {
  handler_t cb = use_int;
  if (a && b)
    cb = use_dbl;
  return cb(x);
}

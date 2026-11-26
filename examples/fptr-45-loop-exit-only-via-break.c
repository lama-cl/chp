// fptr-45-loop-exit-only-via-break.c
//
// Fact F45 (CFG, loops; round 4): an infinite loop whose only exit is a
// `break` taken right after cb = use_dbl.  On every path that reaches cb(x),
// cb is use_dbl, so x is exactly double*.  The AST engine traverses the body
// once and joins nothing at break, so this is resolved by accident or not at
// all; a CFG gets it by construction (08-cfg-fptr-resolution.md, M2a).
//
// Today (measured): UnguardedMultiTarget double*,int* (imprecise: use_int cannot reach the call).
//
// @fptr-fact F45 loop-exit-only-via-break
// @subject   run::x
// @baseline  category=UnguardedMultiTargetActiveReinterpretation callback=ResolvedCallback behavior=MultiTypeRead types=double*,int* resolution=set@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int c, void *x) {
  handler_t cb = use_int;
  for (;;) {
    if (c++ > 3) {
      cb = use_dbl;
      break;
    }
  }
  return cb(x);
}

// fptr-51-dispatch-loop-switch.c
//
// Fact F51 (CFG, combination; round 4): a state-machine loop with a switch,
// break, continue and return.  The handler chosen in state 0 is used on a
// later iteration on the same object x: unrelated dispatch over iterations.
//
// Today (measured): exact double* — UNSOUND, case 1 on the first iteration (use_int) is lost.
//
// @fptr-fact F51 dispatch-loop-switch
// @subject   run::x
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  types=double*,int* resolution=set@L0 dispatch=unrelated
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int s, void *x) {
  handler_t cb = use_int;
  for (;;) {
    switch (s) {
    case 0:
      cb = use_dbl;
      s = 1;
      continue;
    case 1:
      if (cb(x) > 0)
        return 1;
      s = 2;
      break;
    default:
      return 0;
    }
  }
}

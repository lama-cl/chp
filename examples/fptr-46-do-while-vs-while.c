// fptr-46-do-while-vs-while.c
//
// Fact F46 (CFG, loops; round 4): a do-while body runs at least once, a while
// body may run zero times.  After `do { cb = use_dbl; } while (c)` the call
// sees exactly use_dbl (x: double*).  After `while (c) { cb = use_dbl; }` it
// sees {use_int, use_dbl}; y is the same object for the whole function but
// only one call happens, so the set is per-path, not a reinterpretation of y
// across iterations: dispatch=single-call (D30), reported as a set.
//
// Today (measured): run_do::x exact double* (correct); run_while::y exact double* — UNSOUND, the zero-iteration path with use_int is lost.
//
// @fptr-fact F46 do-while-vs-while
// @subject   run_do::x
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L0
// @subject   run_while::y
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run_do(int c, void *x) {
  handler_t cb = use_int;
  do {
    cb = use_dbl;
  } while (c-- > 0);
  return cb(x);
}
int run_while(int c, void *y) {
  handler_t cb = use_int;
  while (c-- > 0)
    cb = use_dbl;
  return cb(y);
}

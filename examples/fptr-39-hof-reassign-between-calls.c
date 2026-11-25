// fptr-39-hof-reassign-between-calls.c
//
// Fact F39 (control, supported today): the same local fptr is reassigned
// between two calls of a higher-order function.  hof's summary is parametric
// ($1 := $0.$0); each direct call site must specialise it with the target
// current at that site: demo::a -> int*, demo::b -> double*.
//
// Origin: owner probes examples/probes/fptr-2.c and fptr-hof-reassign-hof.c
// (identical; corrected: hof takes handler_t, not handler_t *).
//
// @fptr-fact F39 hof-reassign-between-calls
// @subject   demo::a
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int*
// @subject   demo::b
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=double*
// @subject   hof::data
// @baseline  category=OpaqueTransportOrViewOnly callback=FormalCallbackParameter behavior=CallbackDependentPointer types=- resolution=parametric@L0 sigmismatch=false
// @expected  category=OpaqueTransportOrViewOnly callback=FormalCallbackParameter resolution=parametric@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
static int hof(handler_t pf, void *data) { return pf(data); }
int demo(void *a, void *b) {
  handler_t fp = 0;
  int r = 0;
  fp = use_int;
  r += hof(fp, a);
  fp = use_dbl;
  r += hof(fp, b);
  return r;
}

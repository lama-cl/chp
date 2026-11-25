// fptr-38-strong-update-between-calls.c
//
// Fact F38 (control, supported today): a local fptr is reassigned between two
// indirect calls.  The L0 strong update must give each call exactly its own
// target: demo::a -> int*, demo::b -> double*.  Guards M2 (switch/loop joins)
// and M3 (L1 fallback) against weakening strong updates into joins.
//
// Origin: owner probe examples/probes/fptr-call-reassign-call.c (corrected:
// a void* subject is passed instead of &i / &c, so the caller has a subject).
//
// @fptr-fact F38 strong-update-between-calls
// @subject   demo::a
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
// @subject   demo::b
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double*
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int demo(void *a, void *b) {
  handler_t fp = 0;
  int r = 0;
  fp = use_int;
  r += fp(a);
  fp = use_dbl;
  r += fp(b);
  return r;
}

// fptr-25-fptr-in-union.c
//
// Fact F25 (control, supported today): a function pointer stored in a union
// member (tagged callback variants).  The store goes through u.cb.fn; the call reads the same member.
// Union members alias each other, so the storage key must be the union slot;
// keep this passing when L1 storage keys are introduced.
//
// Real world: thttpd's ClientData is a union passed alongside TimerProc
// (test_projects/bench-3c/thttpd-2.29/timers.h:41).
//
// @fptr-fact F25 fptr-in-union
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedFieldCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

union action {
  handler_t fn;
  long code;
};

int run(void *ctx) {
  union action a;
  a.fn = use_int;
  return a.fn(ctx);
}

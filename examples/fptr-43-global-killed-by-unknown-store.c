// fptr-43-global-killed-by-unknown-store.c
//
// Fact F43 (L0 tri-state guard): a global fptr with a known initializer is
// overwritten with a value from an undefined (external) function.
//   * run_local: the unknown store happens in the same function, right before
//     the call.  L0 must record "killed-unknown", and the L1 fallback must NOT
//     resurrect the initializer target.  Correct today; guards M3a.
//   * run: the unknown store happens in another function (load).  The slot is
//     {use_int} U T(external) = T, so the call is unresolved.  Today the
//     initializer is used as the current value (same defect as F35), giving an
//     unsound int*-only answer.
//
// @fptr-fact F43 global-killed-by-unknown-store
// @subject   run_local::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=OpaqueTransportOrViewOnly resolution=unresolved@-
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=OpaqueTransportOrViewOnly resolution=unresolved@-
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
extern handler_t lookup_plugin(const char *name);
static handler_t g_handler = use_int;
int run_local(void *ctx) {
  g_handler = lookup_plugin("x");
  return g_handler(ctx);
}
void load(void) { g_handler = lookup_plugin("y"); }
int run(void *ctx) { return g_handler(ctx); }

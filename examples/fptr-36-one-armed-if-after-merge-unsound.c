// fptr-36-one-armed-if-after-merge-unsound.c
//
// Fact F36 (soundness bug, found in review round 2): an if/else gives cb a
// two-target set {use_int, use_dbl}; a later one-armed `if (b) cb = use_lng;`
// must join to {use_int, use_dbl, use_lng}.  Today only use_lng survives.
//
// Root cause: TraverseIfStmt (CastHistoryVisitor.h:3074) snapshots the
// single-target map functionPointerTargets but not functionPointerTargetSets,
// and mergeFunctionPointerStates (:25352) derives the joined set from the
// single-target maps only.  A multi-target set that exists before the if is
// invisible to the merge, so the else path (cb unchanged) contributes nothing.
// A related guard defect: with a single prior target the untouched path is
// labelled [always] instead of [!(b)].
//
// Origin: owner probe examples/probes/fptr-call-reassign-call.c generalised
// to branch joins (see design/fptr-extension/05-plan-review.md, B1).
//
// @fptr-fact F36 one-armed-if-after-merge-unsound
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=long*
// @expected  category=GuardedActiveReinterpretation types=double*,int*,long* resolution=set@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
static int use_lng(void *p) { long *q = p; return (int)*q; }
int run(int a, int b, void *ctx) {
  handler_t cb = 0;
  if (a)
    cb = use_int;
  else
    cb = use_dbl;
  if (b)
    cb = use_lng;
  return cb(ctx);
}

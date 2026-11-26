// fptr-44-early-return-merge-imprecise.c
//
// Fact F44 (precision, found in review round 3): the then-branch of an `if`
// that ends in `return` cannot reach the code after the `if`, but the engine
// has no CFG: TraverseIfStmt (CastHistoryVisitor.h) merges the then-state into
// the join exactly as if control fell through.  So run::b, reached only when
// !x, is reported with the infeasible target use_dbl under guard [x], giving a
// false multi-target (Guarded) answer.  Imprecise, not unsound: no reachable
// target is lost.  The same holds for break/continue inside an if.
//
// Required (D23): a branch ending in return/break/continue contributes nothing
// to the join for fptr state (seam S5 extension), or option (c) (CFG).
// See design/fptr-extension/07-existing-engine.md.
//
// @fptr-fact F44 early-return-merge-imprecise
// @subject   run::b
// @baseline  category=UnguardedMultiTargetActiveReinterpretation callback=ResolvedCallback behavior=MultiTypeRead types=double*,int* resolution=set@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int x, void *a, void *b) {
  handler_t cb = use_int;
  if (x) {
    cb = use_dbl;
    return cb(a);
  }
  return cb(b);
}

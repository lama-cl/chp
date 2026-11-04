// history-risk-parallel-sequential-moderate.c
//
// Purpose:
//   historyRisk is structural and independent of pointer operations.
//
// Expected direct history:
//   x : void * => ⟨| int *, char * => double * |⟩
//
// Expected pointer behavior metrics:
//   x
//     historyShape: ParallelPolymorphic
//     historyRisk: Moderate
//     behavior: ParallelSequentialReinterpretationNoDeref
//
// Rationale:
//   There is no dereference/write, so behavior is NoDeref.
//   The cast history is still structurally non-low because it has independent
//   non-void target views and a parallel sequential arm.
void reinterpret_many(void *x) {
  int *pi = x;
  char *pc = x;
  double *pd = (double *)pc;
  (void)pi;
  (void)pd;
}

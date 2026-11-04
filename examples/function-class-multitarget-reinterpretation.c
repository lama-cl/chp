// function-class-multitarget-reinterpretation.c
//
// Purpose:
//   Function-level classification for non-branch multi-target reinterpretation.
//
// Expected function behavior classification:
//   reinterpret_many
//     class: MultiTargetReinterpretationFunction
//
// Rationale:
//   The void* parameter reaches multiple concrete target types without a
//   branch-guarded tagged-union structure.
void reinterpret_many(void *x) {
  int *pi = x;
  char *pc = x;
  double *pd = (double *)pc;
}

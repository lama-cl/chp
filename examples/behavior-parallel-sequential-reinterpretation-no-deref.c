// behavior-parallel-sequential-reinterpretation-no-deref.c
//
// Purpose:
//   A parallel group with an internal sequential arm should be distinguished
//   from a flat multi-target parallel group.
//
// Program pattern:
//   void *pv;
//   int *pi = pv;
//   char *pc = pv;
//   double *pd = (double *)pc;
//
// Expected cast history:
//   pv : void * => ⟨| int *, char * => double * |⟩
//
// Expected behavior:
//   pv
//     behavior: ParallelSequentialReinterpretationNoDeref
//     directionalPatterns: ... parallelSequentialArm=yes ...
//
// pc should be:
//   behavior: ByteViewToConcreteNoDeref
void demo(void *pv) {
  int *pi = pv;
  char *pc = pv;
  double *pd = (double *)pc;
}

// parallel-multitarget-no-deref.c
//
// Purpose:
//   A void* value interpreted as several independent concrete target pointer
//   types should be classified as MultiTargetParallelNoDeref, not
//   ByteViewNoDeref, even if one arm is char*.
//
// Program pattern:
//   void *pv;
//   int    *pi = pv;
//   char   *pc = pv;
//   double *pd = pv;
//
// Expected cast histories:
//   pv : void * => ⟨| int *, char *, double * |⟩
//
// Expected behavior metric:
//   pv
//     behavior: MultiTargetParallelNoDeref
//     byteView=yes
//     multiType: deref=no, write=no, branchSplit=no
//
// Rationale:
//   char* is only one arm.  The dominant pattern is multiple parallel target
//   types with no dereference evidence.
void demo(void *pv) {
  int *pi = pv;
  char *pc = pv;
  double *pd = pv;
}

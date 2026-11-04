// parallel-multitarget-with-byte-arm.c
//
// Purpose:
//   If a parallel cast group contains a char* arm plus other concrete pointer
//   target types, the behavior should emphasize the multi-target parallel
//   structure rather than only the byte view.
//
// Expected:
//   pv : void * => ⟨| char *, short *, long * |⟩
//
// Expected behavior:
//   MultiTargetParallelNoDeref
void demo(void *pv) {
  char *pc = pv;
  short *ps = pv;
  long *pl = pv;
}

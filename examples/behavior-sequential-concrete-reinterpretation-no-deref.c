// behavior-sequential-concrete-reinterpretation-no-deref.c
//
// Purpose:
//   char* => int* => double* should not be classified as
//   ByteMediatedReinterpretationNoDeref, because char* is the source, not the
//   middle mediator.
//
// Expected:
//   c : char * => int * => double *
//   behavior: SequentialConcreteReinterpretationNoDeref
//
// Rationale:
//   After leaving char*, the pointer is sequentially reinterpreted between
//   concrete non-byte pointer types.
void demo(char *c) {
  int *i = (int *)c;
  double *d = (double *)i;
}

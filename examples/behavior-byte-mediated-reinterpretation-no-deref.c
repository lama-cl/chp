// behavior-byte-mediated-reinterpretation-no-deref.c
//
// Purpose:
//   T1* => char* => T2* where T1 != T2 should be classified as
//   ByteMediatedReinterpretationNoDeref.
//
// Expected:
//   p : int * => char * => double *
//   behavior: ByteMediatedReinterpretationNoDeref
//
// Rationale:
//   char* is the middle byte-view mediator between two concrete pointer types.
void demo(int *p) {
  char *c = (char *)p;
  double *d = (double *)c;
}

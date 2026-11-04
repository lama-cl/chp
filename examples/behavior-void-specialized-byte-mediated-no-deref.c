// behavior-void-specialized-byte-mediated-no-deref.c
//
// Purpose:
//   void* => int* => char* => double* should also be classified as
//   ByteMediatedReinterpretationNoDeref, with voidInterface=yes as a secondary
//   source-kind flag.
//
// Expected:
//   pv : void * => int * => char * => double *
//   behavior: ByteMediatedReinterpretationNoDeref
//   sourceKinds: voidInterface=yes
void demo(void *pv) {
  int *i = pv;
  char *c = (char *)i;
  double *d = (double *)c;
}

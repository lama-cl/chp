/*
Expected C4O-Prov behavior:

This example is intended to exercise display-only call-site provenance.  The
solver/core summaries should remain wrapper-free.  The final import/report run
may include a section like:

  Project-solved call-site provenance:
    use_int.$0 -> poly.$0 : int * => poly⌜void * => int *⌝poly
    use_double.$0 -> poly.$0 : double * => poly⌜void * => double *⌝poly
    use_s.$0 -> poly.$0 : struct S * => poly⌜void * => struct S *⌝poly

This supports the interpretation that poly is used in a type-preserving way at
these call sites.  Pointer-use summaries still need to be considered before
classifying the function as truly safe polymorphic code.
*/

struct S { int x; };

void poly(void *x, int tag) {
  if (tag == 0) {
    int *p = x;
    (void)p;
  } else if (tag == 1) {
    double *p = x;
    (void)p;
  } else {
    struct S *p = x;
    (void)p;
  }
}

void use_int(int *p) {
  poly(p, 0);
}

void use_double(double *p) {
  poly(p, 1);
}

void use_s(struct S *p) {
  poly(p, 2);
}

/*
Expected C4O-Prov behavior:

This example is intended to show why call-site-specialized provenance is more
useful than a single global summary.  A report-domain view may include:

  Project-solved call-site provenance:
    use_int.$0 -> force_double.$0 : int * => force_double⌜void * => double *⌝force_double
    use_double.$0 -> force_double.$0 : double * => force_double⌜void * => double *⌝force_double

The first line suggests that force_double is not type-preserving for int* call
sites.  It reinterprets the input as double*.  Pointer-use summaries should be
combined with this evidence when deciding whether the function is unsafe or
simply forwarding/storing values.
*/

void force_double(void *x) {
  double *p = x;
  (void)p;
}

void use_int(int *p) {
  force_double(p);
}

void use_double(double *p) {
  force_double(p);
}

// GATE ext-libc-dataflow
// Pointer values flow *through* external functions whose data-flow contract
// is known: memcpy copies the stored pointer value, realloc returns (an alias
// of) its argument.  No cast and no assignment connects source and sink.
//
// Probe subjects:
//   read_copied.$0 (p : void *) -> memcpy(&out, &p) -> out read as int *
//   grow.$0 (buf : void *)      -> realloc          -> result read as double *
#include <stdlib.h>
#include <string.h>

int read_copied(void *p) {
  void *out;
  memcpy(&out, &p, sizeof out);
  int *pi = (int *)out;
  return *pi;
}

double *grow(void *buf, size_t n) {
  void *bigger = realloc(buf, n * sizeof(double));
  double *pd = (double *)bigger;
  if (pd)
    pd[0] = 0.0;
  return pd;
}

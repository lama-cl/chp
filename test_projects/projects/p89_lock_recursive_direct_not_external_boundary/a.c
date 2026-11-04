/*
 * p89_lock_recursive_direct_not_external_boundary/a.c
 *
 * C6f1 classification-lock test: local recursive direct call is not an
 * external boundary.
 */

void f(void *x, int n) {
  if (n <= 0)
    return;

  char *pc = x;
  void *pv = pc;
  int *pi = pv;
  *pi += 1;

  f(pi, n - 1);
}

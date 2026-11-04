/*
 * p124_mutual_three_recursion_no_if_widening/a.c
 *
 * C6g5d1 regression: an ordinary three-function recursive SCC has a guard
 * statement, but no recursive call occurs while its if condition is visited.
 * It must follow the established C6g5c1 SCC path and never receive a
 * rec{same-tu-if-scc|...} widening marker.
 */

void g(void *y, int n);
void h(void *z, int n);

void f(void *x, int n) {
  if (n <= 0) return;
  int *pi = x;
  g(pi, n - 1);
}

void g(void *y, int n) {
  char *pc = y;
  h(pc, n - 1);
}

void h(void *z, int n) {
  double *pd = z;
  f(pd, n - 1);
}

int run_mutual_three(void *x) {
  f(x, 3);
  return 0;
}

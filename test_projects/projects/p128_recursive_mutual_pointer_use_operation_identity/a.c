/*
 * p128_recursive_mutual_pointer_use_operation_identity/a.c
 *
 * C6g5r regression: cast history reaches its ordinary recursive SCC
 * fixpoint, then pointer operations are propagated as a relation-sensitive
 * set of immutable operation seeds.  Replayed edges must not create a new
 * pointer-use fact merely because its attached cast-history report path grows.
 */

void g(void *y, int n);

void f(void *x, int n) {
  if (n <= 0)
    return;
  int *px = x;
  g(px, n - 1);
}

void g(void *y, int n) {
  char *pc = y;
  *pc += 1;
  f(pc, n - 1);
}

int run_recursive_mutual_pointer_use_operation_identity(void *p) {
  f(p, 3);
  return 0;
}

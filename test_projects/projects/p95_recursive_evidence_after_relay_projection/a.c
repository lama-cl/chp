// C6g4n1 regression: direct recursion retains recursive evidence after relay projection.
void f(void *x, int n) {
  if (n <= 0) return;
  char *pc = x;
  *pc += 1;
  f(x, n - 1);
}

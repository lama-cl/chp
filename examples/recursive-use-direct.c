// recursive-use-direct.c
void f(void *x, int n) {
  if (n <= 0) return;
  char *pc = x;
  *pc += 1;
  f(pc, n - 1);
}

// recursive-use-mutual.c
void g(void *y, int n);

void f(void *x, int n) {
  if (n <= 0) return;
  int *pi = x;
  g(pi, n - 1);
}

void g(void *y, int n) {
  char *pc = y;
  *pc += 1;
  f(pc, n - 1);
}

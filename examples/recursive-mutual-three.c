// recursive-mutual-three.c
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

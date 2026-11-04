// recursive-direct.c
void f(void *x, int n) {
  if (n <= 0) return;
  char *pc = x;
  void* pv = pc;
  int* pi = pv;
  *pi += 1;
  f(pi, n - 1);
}

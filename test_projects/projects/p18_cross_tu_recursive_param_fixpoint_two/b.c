extern void f(void *x);

void g(void *x) {
  int *pi = x;
  f(pi);
}

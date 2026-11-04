extern void f(void *);

void g(void *y) {
  int *p = y;
  *p += 1;
  f(y);
}

/*
 * p02_cross_tu_return/a.c
 *
 * f returns g(x), and g is defined in b.c.
 */

void *g(void *);

void *f(void *x) {
  return g(x);
}

int main(void) {
  int i = 0;
  void *r = f(&i);
  return r == 0;
}

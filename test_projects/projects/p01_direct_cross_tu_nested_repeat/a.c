/*
 * p01_direct_cross_tu/a.c
 *
 * f calls g, but g is defined in b.c.
 * Pass 1 should export a local summary for f with an unresolved/importable
 * direct call to g.
 * C3 should import g's summary and project g's int* write behavior onto f::x.
 */

void g(void *);

void f(void *x) {
  g(x);
}

int main(void) {
  int i = 0;
  f(&i);
  return i;
}

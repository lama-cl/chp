/*
 * p03_cross_tu_mutual_recursion/b.c
 *
 * g calls f, which is defined in a.c.
 */

void f(void *);

void g(void *y) {
  f(y);
}

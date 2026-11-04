/*
 * p03_cross_tu_mutual_recursion/a.c
 *
 * f and g mutually recurse across TUs.
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

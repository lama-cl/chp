/*
 * C3b-4 regression: function-pointer return summary propagated through a field.
 *
 * Expected:
 *   - fp is resolved to g
 *   - g's imported return alias becomes the reaching origin of b.p
 *   - return b.p preserves a connection to $0 and the imported g⌜...⌝g path
 */

struct Box {
  void *p;
};

void *g(void *);

void *f(void *x) {
  void *(*fp)(void *) = g;
  struct Box b;
  b.p = fp(x);
  return b.p;
}

int main(void) {
  int i = 0;
  void *r = f(&i);
  return r == 0;
}

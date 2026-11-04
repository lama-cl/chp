/*
 * C3b-4 regression: direct return through a known function-pointer target.
 *
 * Expected:
 *   - fp is resolved to g
 *   - parameter cast summary of g is projected onto f::x
 *   - return summary of g is projected onto f's return summary
 *   - ret aliases $0
 */

void *g(void *);

void *f(void *x) {
  void *(*fp)(void *) = g;
  return fp(x);
}

int main(void) {
  int i = 0;
  void *r = f(&i);
  return r == 0;
}

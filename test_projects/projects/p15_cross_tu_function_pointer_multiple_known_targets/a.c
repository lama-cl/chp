/*
 * C3c-1 regression: branch-merged function pointer with multiple known targets.
 */
void g1(void *);
void g2(void *);

void f(void *x, int cond) {
  void (*fp)(void *);
  if (cond)
    fp = g1;
  else
    fp = g2;
  fp(x);
}

int main(void) {
  int i = 0;
  f(&i, 1);
  return i;
}

extern void *g1(void *x);
extern void *g2(void *x);

void *f(void *x, int cond) {
  void *(*fp)(void *);
  if (cond)
    fp = g1;
  else
    fp = g2;
  return fp(x);
}

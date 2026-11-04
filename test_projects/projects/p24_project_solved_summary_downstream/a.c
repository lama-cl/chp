extern void g(void *x);

static void relay(void *x) {
  g(x);
}

void f(void *x) {
  char *pc = x;
  relay(pc);
}

void h(void *p) {
  f(p);
}

void k(void *q) {
  h(q);
}

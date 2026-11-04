extern void g(void *x);

static void relay(void *x) {
  g(x);
}

void f(void *x) {
  char *pc = x;
  pc += 4;
  *pc += 1;
  relay(pc);
}

// C6g4o1 regression: cross-TU project-SCC recursion keeps recursive evidence.
extern void g(void *x);

static void relay(void *x) {
  g(x);
}

void f(void *x) {
  char *pc = x;
  relay(pc);
}

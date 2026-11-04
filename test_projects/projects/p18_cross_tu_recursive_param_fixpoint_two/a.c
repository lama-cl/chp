extern void g(void *x);


void f(void *x) {
  char* pc = x;
  g(pc);
}

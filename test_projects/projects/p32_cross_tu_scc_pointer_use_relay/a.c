extern void g(void *);

static void relay(void *r) {
  g(r);
}

void f(void *x) {
  relay(x);
}

int main(void) {
  int i = 0;
  f(&i);
  return i;
}

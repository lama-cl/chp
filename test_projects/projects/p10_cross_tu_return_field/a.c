struct Box { void *p; };

void *g(void *);

void *f(void *x) {
  struct Box b;
  b.p = g(x);
  return b.p;
}

int main(void) { return f((void *)0) != (void *)0; }

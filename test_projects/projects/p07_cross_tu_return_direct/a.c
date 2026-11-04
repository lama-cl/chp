void *g(void *);

void *f(void *x) {
  return g(x);
}

int main(void) { return f((void *)0) != (void *)0; }

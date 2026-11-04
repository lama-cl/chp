void *g(void *);

void *f(void *x) {
  void *r = g(x);
  return r;
}

int main(void) { return f((void *)0) != (void *)0; }

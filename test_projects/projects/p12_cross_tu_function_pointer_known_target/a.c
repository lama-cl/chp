void g(void *);

void f(void *x) {
  void (*fp)(void *) = g;
  fp(x);
}

int main(void) {
  return 0;
}

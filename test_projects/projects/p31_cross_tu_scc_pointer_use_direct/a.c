extern void g(void *);

void f(void *x) {
  g(x);
}

int main(void) {
  int i = 0;
  f(&i);
  return i;
}

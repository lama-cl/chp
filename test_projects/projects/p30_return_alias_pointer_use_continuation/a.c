// p30_return_alias_pointer_use_continuation/a.c
// C4Q: pointer-use propagation through an imported returned alias.

void *idv(void *);

void use(void *y) {
  void *z = idv(y);
  int *p = z;
  *p += 1;
}

int main(void) {
  int i = 0;
  use(&i);
  return i;
}

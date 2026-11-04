// p30_return_alias_pointer_use_continuation/b.c

void *idv(void *x) {
  int *p = x;
  return p;
}

/*
 * p05_function_pointer_target/b.c
 *
 * idv is the callback target. ho calls through the function pointer.
 */

void *idv(void *y) {
  int *p = y;
  *p += 1;
  return p;
}

void ho(void *(*pf)(void *), void *x) {
  char *pc = x;
  pf(pc);
}

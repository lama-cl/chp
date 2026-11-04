void *idv(void *x) {
  int *p = x;
  *p = *p + 1;
  return p;
}

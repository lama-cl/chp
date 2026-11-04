void *g(void *y) {
  char *pc = y;
  void *pv = pc;
  int *p = y;
  *p += 1;
  return pv;
}

void *g1(void *x) {
  int *p = x;
  *p = *p + 1;
  return p;
}

void *g2(void *x) {
  char *p = x;
  *p += 1;
  return p;
}

void g1(void *y) {
  int *p = y;
  *p += 1;
}

void g2(void *y) {
  char *p = y;
  *p += 1;
}

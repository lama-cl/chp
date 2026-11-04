extern void *idv(void *x);

void use(void *y) {
  void *z = idv(y);
  char *c = z;
  (void)c;
}

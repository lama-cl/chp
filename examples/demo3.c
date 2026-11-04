int main(void) {
  unsigned long address = 0x40001000;
  int *ptr = (int *)address;
  int i = 0, j=0;
  int *pi1 = &i;
  int *pj1 = &j;
  void *pv = pi1;
  pv = pj1;
  pv = ptr;
  char *pc1 = pv;
  return i;
}

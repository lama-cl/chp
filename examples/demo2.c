int main(void) {
  int i = 0, j=0;
  int *pi1 = &i;
  int *pj1 = &j;
  void *pv = pi1;
  pv = pj1;
  char *pc1 = pv;

  int *pi2 = &i;
  int *pj2 = &j;
  void* pv2;
  if (i > 0) {
    pv2 = pi2;
  } else {
    pv2 = pj2;
  }

  char* pc2 = pv2;

  return i;
}

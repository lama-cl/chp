void *fret(void *pvf) {
  char *pc = pvf;
  *pc += 1;
  return pvf;
}

void *idv(void *x) {
  int *pidv = x;
  *pidv = *pidv + 1;
  return pidv;
}

void ho(void *(*pf)(void *), void *pvho) {
  char *pcho = pvho;
  pf(pcho);
}

int main(void) {
  int i = 0;
  int *pi1 = &i;
  void *pv = pi1;

  void *(*pfunc)(void*) = idv;
  ho(pfunc, pv);

  pfunc = fret;
  ho(pfunc, pv);
  
  return i;
}
void f(void *pvf) {
  char *pc = pvf;
  *pc += 1;
}

void *idv(void *x) {
  int *pidv = x;
  *pidv = *pidv + 1;
  return pidv;
}

void ho(void *(*pf)(void *), void *pvho) {
  char* pcho = pvho;
  pf(pcho);
}

int main(void) {
  int i = 0;
  int *pi1 = &i;
  void *pv = pi1;
  char *pi2 = pv;
  *pi2 += 1;
  f(pv);
  ho(idv, pv);
  // ho(f,pv);

  if (i > 0) {
    char *pc = pv;
    *pc += 1;
  } else {
    int *pi3 = pv;
    *pi3 += 2;
  }

  return i;
}

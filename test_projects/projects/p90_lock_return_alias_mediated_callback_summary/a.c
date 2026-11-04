/*
 * p90_lock_return_alias_mediated_callback_summary/a.c
 *
 * C6f1 classification-lock test: return-alias evidence remains visible when
 * combined with callback/higher-order mediation.
 *
 * This test intentionally uses if/else function-pointer assignment instead of
 * a conditional expression. Conditional-operator target-set recovery is a
 * separate language-feature extension and should not be required by this
 * regression lock.
 */

void *idv(void *x) {
  int *pi = x;
  *pi += 1;
  return pi;
}

void *fret(void *x) {
  char *pc = x;
  *pc += 1;
  return x;
}

void ho(void *(*pf)(void *), void *pvho) {
  char *pcho = pvho;
  pf(pcho);
}

void demo(int tag) {
  int i = 0;
  int *pi = &i;
  void *pv = pi;
  void *(*pf)(void *) = 0;

  if (tag)
    pf = idv;
  else
    pf = fret;

  ho(pf, pv);
}

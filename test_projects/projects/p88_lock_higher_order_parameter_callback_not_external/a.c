/*
 * p88_lock_higher_order_parameter_callback_not_external/a.c
 *
 * C6f1 classification-lock test: function-parameter callback should not be
 * reported as an external direct call.
 */

void *idv(void *x) {
  int *pi = x;
  *pi += 1;
  return pi;
}

void ho(void *(*pf)(void *), void *pvho) {
  char *pcho = pvho;
  pf(pcho);
}

void demo(void) {
  int i = 0;
  int *pi = &i;
  void *pv = pi;
  ho(idv, pv);
}

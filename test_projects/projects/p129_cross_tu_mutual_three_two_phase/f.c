/* C6g5s cross-TU three-function cycle: cast phase must use imported facts. */
#include "api.h"

void p129_f(void *x, int n) {
  if (n <= 0)
    return;
  int *pi = x;
  p129_g(pi, n - 1);
}

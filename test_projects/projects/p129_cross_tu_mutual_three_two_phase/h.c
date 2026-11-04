#include "api.h"

void p129_h(void *z, int n) {
  if (n <= 0)
    return;
  double *pd = z;
  p129_f(pd, n - 1);
}

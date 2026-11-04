#include "api.h"

void p129_g(void *y, int n) {
  if (n <= 0)
    return;
  char *pc = y;
  p129_h(pc, n - 1);
}

/* C6g5s cross-TU counterpart of p126. */
#include "api.h"

void p130_relay_a(void *p, unsigned remaining) {
  int *as_int = (int *)p;
  char *as_char = (char *)p;
  if (remaining == 0)
    return;

  p130_relay_b(as_int, remaining - 1);
  p130_relay_b(as_char, remaining - 1);
}

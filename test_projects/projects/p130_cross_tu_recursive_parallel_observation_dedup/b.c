#include "api.h"

void p130_relay_b(void *p, unsigned remaining) {
  short *as_short = (short *)p;
  if (remaining == 0)
    return;

  p130_relay_a(as_short, remaining - 1);
}

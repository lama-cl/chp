#include "api.h"

int p130_run(void *p) {
  p130_relay_a(p, 2);
  return 0;
}

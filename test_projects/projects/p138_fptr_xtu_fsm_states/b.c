#include "state.h"

static struct state st_done(void *p) {
  int *q = p;
  struct state s = { 0 };
  *q = 0;
  return s;
}

struct state st_start(void *p) {
  int *q = p;
  struct state s = { st_done };
  *q = 1;
  return s;
}

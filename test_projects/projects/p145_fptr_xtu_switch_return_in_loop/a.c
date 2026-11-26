/*
 * Fact X14 (cross-TU, CFG + RET summary; round 4): lookup() in a.c selects
 * its return target with a switch; b.c calls lookup() inside a loop and
 * dispatches the same object x through the result on every iteration.
 * Needs a CFG-derived return-target summary for lookup ({use_int, use_dbl})
 * exported cross-TU (D27), and the loop makes the dispatch unrelated (D30).
 */
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

handler_t lookup(int k) {
  switch (k) {
  case 0:
    return use_int;
  default:
    return use_dbl;
  }
}

/*
 * p126_recursive_parallel_observation_dedup/a.c
 *
 * C6g5q regression: two distinct cast views are transferred through an
 * ordinary mutual-recursive SCC outside if-condition evaluation. The cast
 * phase must reach semantic fixpoint without using pointer-use insertions as
 * a reason to reopen cast-history solving.
 */

static void relay_b(void *p, unsigned remaining);

static void relay_a(void *p, unsigned remaining) {
  int *as_int = (int *)p;
  char *as_char = (char *)p;
  if (remaining == 0)
    return;

  relay_b(as_int, remaining - 1);
  relay_b(as_char, remaining - 1);
}

static void relay_b(void *p, unsigned remaining) {
  short *as_short = (short *)p;
  if (remaining == 0)
    return;

  relay_a(as_short, remaining - 1);
}

int run_recursive_parallel_observations(void *p) {
  relay_a(p, 2);
  return 0;
}

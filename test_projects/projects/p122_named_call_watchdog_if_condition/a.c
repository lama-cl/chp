/*
 * p122_named_call_watchdog_if_condition/a.c
 *
 * C6g5c regression fixture: the recursive named direct call occurs in an
 * if-condition.  This exercises condition-context capture without changing
 * cast-history or pointer-use membership.
 */

static int watchdog_recursive(unsigned char *p, unsigned remaining) {
  if (remaining == 0)
    return *p;

  if (watchdog_recursive(p + 1, remaining - 1))
    return *p;

  return 0;
}

int run_watchdog_recursive(unsigned char *p) {
  return watchdog_recursive(p, 2);
}

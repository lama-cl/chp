/*
 * p123_recursive_conditional_summary_collapse/a.c
 *
 * C6g5d1 regression: a direct self call occurs in an if condition and carries
 * a non-trivial pointer reinterpretation.  The named-call solver must retain
 * one terminal recursive-condition marker and converge without importing the
 * evolving callee summary through this same-SCC condition edge.
 */

static int recursive_condition_summary(unsigned char *p, unsigned remaining) {
  const void *as_void = (const void *)p;
  if (remaining == 0)
    return as_void != (const void *)0;

  if (recursive_condition_summary((unsigned char *)((const unsigned char *)p + 1),
                                  remaining - 1))
    return as_void != (const void *)0;

  return 0;
}

int run_recursive_condition_summary(unsigned char *p) {
  return recursive_condition_summary(p, 2);
}

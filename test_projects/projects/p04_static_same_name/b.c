/*
 * p04_static_same_name/b.c
 *
 * This file also has a static helper named helper, but it is a distinct
 * source-scoped function.
 */

static void helper(void *x) {
  char *p = x;
  *p += 1;
}

void fb(void *x) {
  helper(x);
}

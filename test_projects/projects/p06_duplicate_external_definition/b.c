/*
 * p06_duplicate_external_definition/b.c
 *
 * Duplicate external definition of dup.
 */

void dup(void *x) {
  char *p = x;
  *p += 1;
}

void fb(void *x) {
  dup(x);
}

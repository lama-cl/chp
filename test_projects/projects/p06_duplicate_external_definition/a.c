/*
 * p06_duplicate_external_definition/a.c
 *
 * This project intentionally has a duplicate external definition of dup.
 * It is not a valid linked C program; it is for testing C2 diagnostics.
 */

void dup(void *x) {
  int *p = x;
  *p += 1;
}

void fa(void *x) {
  dup(x);
}

/*
 * p82_lock_direct_concrete_specialization/a.c
 *
 * C6f1 classification-lock test: direct void* concrete specialization.
 */

void run(void *x) {
  int *p = x;
  *p += 1;
}

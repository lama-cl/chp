/*
 * p76_classify_direct_void_specialization/a.c
 *
 * Direct concrete specialization of a void * parameter.
 */

void run(void *x) {
  int *p = x;
  *p += 1;
}

/*
 * p81_classify_external_unknown_boundary/a.c
 *
 * The void * parameter crosses an unresolved external boundary.
 */

extern void sink(void *x);

void run(void *x) {
  sink(x);
}

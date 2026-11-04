/*
 * p87_lock_external_unknown_boundary/a.c
 *
 * C6f1 classification-lock test: true unresolved external boundary.
 */

extern void sink(void *x);

void run(void *x) {
  sink(x);
}

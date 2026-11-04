/*
 * p48_field_function_pointer_mixed_local_cross_tu_targets_recursive/a.c
 *
 * Adapted from p43, but the local same-TU field-function-pointer target and
 * the external cross-TU target call each other recursively.  C6c1 must not
 * keep the local raw first projection (int *) as a parallel sibling next to the
 * richer local_handle⌜int * => void * => char * => void *⌝local_handle wrapper
 * for the same guarded field-function-pointer call edge.
 */

struct Ops {
  void (*process)(void *);
};

extern void external_handle(void *x);

void local_handle(void *x) {
  int *p = x;
  *p += 1;
  external_handle(p);
}

void run(struct Ops *ops, void *x, int tag) {
  if (tag)
    ops->process = local_handle;
  else
    ops->process = external_handle;
  ops->process(x);
}

/*
 * p43_field_function_pointer_mixed_local_cross_tu_targets/a.c
 *
 * One guarded field-function-pointer target is local, while the other is
 * defined in a different translation unit.  C5f2 must keep the local target on
 * the same-TU guarded path and import only the external target alternative.
 */

struct Ops {
  void (*process)(void *);
};

void local_handle(void *x) {
  int *p = x;
  *p += 1;
}

extern void external_handle(void *x);

void run(struct Ops *ops, void *x, int tag) {
  if (tag)
    ops->process = local_handle;
  else
    ops->process = external_handle;
  ops->process(x);
}

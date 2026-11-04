/*
 * p48_field_function_pointer_mixed_local_cross_tu_targets_recursive/b.c
 */

extern void local_handle(void *x);

void external_handle(void *x) {
  char *p = x;
  *p += 1;

  local_handle(p);
}

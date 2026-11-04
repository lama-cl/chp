/*
 * p36_field_function_pointer_targets_branch_set/a.c
 *
 * C5a should record guarded field target facts for branch assignments.
 */

struct Ops {
  void (*process)(void *);
};

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void handle_char(void *x) {
  char *p = x;
  *p += 1;
}

void run(struct Ops *ops, void *x, int tag) {
  if (tag)
    ops->process = handle_int;
  else
    ops->process = handle_char;
  ops->process(x);
}

/*
 * p41_field_function_pointer_struct_copy_assignment/a.c
 *
 * C5e should copy field-sensitive target sets through a record assignment.
 */

struct Ops {
  void (*process)(void *);
};

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void run(void *x) {
  struct Ops base = { handle_int };
  struct Ops local;
  local = base;
  local.process(x);
}

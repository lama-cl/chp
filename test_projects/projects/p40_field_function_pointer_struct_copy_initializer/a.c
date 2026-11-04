/*
 * p40_field_function_pointer_struct_copy_initializer/a.c
 *
 * C5e should copy field-sensitive target sets through a record initializer:
 * local = base, so local.process(x) should inherit base.process's target.
 */

struct Ops {
  void (*process)(void *);
};

void handle_char(void *x) {
  char *p = x;
  *p += 1;
}

struct Ops base = { handle_char };

void run(void *x) {
  struct Ops local = base;
  local.process(x);
}

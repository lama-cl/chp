/*
 * p35_field_function_pointer_targets_initializer/a.c
 *
 * C5a should record field function-pointer targets from aggregate initializers.
 */

struct Ops {
  void (*process)(void *);
};

void handle_char(void *x) {
  char *p = x;
  *p += 1;
}

void run(void *x) {
  struct Ops ops = { handle_char };
  ops.process(x);
}

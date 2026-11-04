/*
 * p34_field_function_pointer_targets_assignment/a.c
 *
 * C5a should record that ops.process is assigned the known target handle_int.
 * C5a is reporting-only: the later call through ops.process is not resolved yet.
 */

struct Ops {
  void (*process)(void *);
};

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void run(void *x) {
  struct Ops ops;
  ops.process = handle_int;
  ops.process(x);
}

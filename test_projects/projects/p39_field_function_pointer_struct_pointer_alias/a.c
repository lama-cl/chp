/*
 * p39_field_function_pointer_struct_pointer_alias/a.c
 *
 * C5e should reuse C5a/C5c field targets through a simple struct-pointer
 * alias: p is bound to &ops, so p->process(x) should resolve as ops.process(x).
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
  struct Ops *p = &ops;
  p->process(x);
}

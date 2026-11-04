/*
 * p37_field_function_pointer_call_import_cross_tu/a.c
 *
 * C5b should consume the C5a field-sensitive target fact for ops.process
 * and import handle_int's summary across the TU boundary at ops.process(x).
 */

struct Ops {
  void (*process)(void *);
};

extern void handle_int(void *x);

void run(void *x) {
  struct Ops ops;
  ops.process = handle_int;
  ops.process(x);
}

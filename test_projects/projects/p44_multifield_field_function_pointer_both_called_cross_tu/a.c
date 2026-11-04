/*
 * p44_multifield_field_function_pointer_both_called_cross_tu/a.c
 *
 * Two different function-pointer fields with the same signature are assigned
 * different cross-TU targets and both are called on the same data pointer.
 * The target sets must remain field-path-sensitive; the two alternatives
 * should arise only because both fields are actually called.
 */

struct Ops {
  void (*parse)(void *);
  void (*emit)(void *);
};

extern void parse_int(void *x);
extern void emit_char(void *x);

void run(void *x) {
  struct Ops ops;
  ops.parse = parse_int;
  ops.emit = emit_char;
  ops.parse(x);
  ops.emit(x);
}

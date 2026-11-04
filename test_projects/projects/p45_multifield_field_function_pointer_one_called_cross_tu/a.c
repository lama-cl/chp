/*
 * p45_multifield_field_function_pointer_one_called_cross_tu/a.c
 *
 * Two function-pointer fields are assigned, but only one field is called.
 * The unused field's target must not leak into x.
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
}

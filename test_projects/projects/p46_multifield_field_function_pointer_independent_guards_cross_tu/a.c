/*
 * p46_multifield_field_function_pointer_independent_guards_cross_tu/a.c
 *
 * Two function-pointer fields have the same signature. One field has guarded
 * alternatives and is called with x. The other field has a fixed target and is
 * called with y. The two fields must not share target alternatives.
 */

struct Ops {
  void (*parse)(void *);
  void (*emit)(void *);
};

extern void parse_int(void *x);
extern void parse_char(void *x);
extern void emit_double(void *x);

void run(struct Ops *ops, void *x, void *y, int tag) {
  if (tag) {
    ops->parse = parse_int;
  } else {
    ops->parse = parse_char;
  }

  ops->emit = emit_double;

  ops->parse(x);
  ops->emit(y);
}

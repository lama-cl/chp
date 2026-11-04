/*
 * p38_field_function_pointer_call_guarded_cross_tu/a.c
 *
 * C5b should use guarded C5a field target facts when importing summaries
 * through an indirect call via ops.process(x).
 */

struct Ops {
  void (*process)(void *);
};

extern void handle_int(void *x);
extern void handle_char(void *x);

void run(int tag, void *x) {
  struct Ops ops;
  if (tag)
    ops.process = handle_int;
  else
    ops.process = handle_char;
  ops.process(x);
}

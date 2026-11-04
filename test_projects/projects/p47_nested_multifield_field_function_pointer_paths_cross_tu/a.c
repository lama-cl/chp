/*
 * p47_nested_multifield_field_function_pointer_paths_cross_tu/a.c
 *
 * Two nested structs contain a function-pointer field with the same field name.
 * Full field paths must be distinguished: ctx.ops.process and
 * ctx.other.process are different storage locations.
 */

struct Ops {
  void (*process)(void *);
};

struct Context {
  struct Ops ops;
  struct Ops other;
};

extern void handle_int(void *x);
extern void handle_char(void *x);

void run(void *x, void *y) {
  struct Context ctx;
  ctx.ops.process = handle_int;
  ctx.other.process = handle_char;

  ctx.ops.process(x);
  ctx.other.process(y);
}

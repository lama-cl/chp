/*
 * p75_nested_data_field_field_fnptr_cross_tu/a.c
 *
 * Nested data-field behavior through a field-function-pointer call whose
 * target summary is imported from another translation unit.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

struct Ops {
  void (*consume)(struct Context *);
};

extern void consume_ctx(struct Context *c);

void run(void *x) {
  struct Context ctx;
  struct Ops ops;

  ctx.box.payload = x;
  ops.consume = consume_ctx;
  ops.consume(&ctx);
}

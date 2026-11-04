/*
 * p74_nested_data_field_field_fnptr_same_tu/a.c
 *
 * Nested data-field behavior through a same-TU field-function-pointer call.
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

void consume_ctx(struct Context *c) {
  int *p = c->box.payload;
  *p += 1;
}

void run(void *x) {
  struct Context ctx;
  struct Ops ops;

  ctx.box.payload = x;
  ops.consume = consume_ctx;
  ops.consume(&ctx);
}

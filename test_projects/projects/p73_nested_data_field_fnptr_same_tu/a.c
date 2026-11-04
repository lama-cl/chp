/*
 * p73_nested_data_field_fnptr_same_tu/a.c
 *
 * Nested data-field behavior through an ordinary same-TU function pointer.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

void consume_ctx(struct Context *c) {
  int *p = c->box.payload;
  *p += 1;
}

void run(void *x) {
  struct Context ctx;
  ctx.box.payload = x;

  void (*cb)(struct Context *) = consume_ctx;
  cb(&ctx);
}

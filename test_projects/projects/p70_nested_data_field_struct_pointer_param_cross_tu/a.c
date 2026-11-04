/*
 * p70_nested_data_field_struct_pointer_param_cross_tu/a.c
 *
 * Cross-TU diagnostic for nested data-field summaries.  The callee observes
 * c->box.payload in b.c.  Import mode for a.c should project that relative
 * field path onto run::ctx.box.payload and then back to the stored origin
 * run::x.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

extern void consume_ctx(struct Context *c);

void run(void *x) {
  struct Context ctx;
  ctx.box.payload = x;
  consume_ctx(&ctx);
}

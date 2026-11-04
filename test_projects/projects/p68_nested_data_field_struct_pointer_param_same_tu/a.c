/*
 * p68_nested_data_field_struct_pointer_param_same_tu/a.c
 *
 * Diagnostic for C6d nested data-field summaries through a struct-pointer
 * parameter.  The callee observes c->box.payload and the caller passes &ctx.
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
  consume_ctx(&ctx);
}

/*
 * p72_guarded_nested_data_field_struct_pointer_param_cross_tu/a.c
 *
 * Cross-TU guarded diagnostic for nested data-field summaries.  Two imported
 * callees interpret the same nested field path c->box.payload differently.
 * The caller-side run::ctx.box.payload and run::x histories should preserve
 * the branch guard, not collapse to an unguarded parallel cast.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

extern void consume_int_ctx(struct Context *c);
extern void consume_char_ctx(struct Context *c);

void run(void *x, int tag) {
  struct Context ctx;
  ctx.box.payload = x;

  if (tag)
    consume_int_ctx(&ctx);
  else
    consume_char_ctx(&ctx);
}

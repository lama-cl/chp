/*
 * p71_nested_data_field_alias_to_inner_struct_cross_tu/a.c
 *
 * Cross-TU diagnostic for aliases into the middle of a nested data-field path.
 * bp aliases &ctx.box.  The imported callee-relative b->payload behavior from
 * b.c should map to run::ctx.box.payload and then back to run::x.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

extern void consume_box(struct Box *b);

void run(void *x) {
  struct Context ctx;
  ctx.box.payload = x;

  struct Box *bp = &ctx.box;
  consume_box(bp);
}

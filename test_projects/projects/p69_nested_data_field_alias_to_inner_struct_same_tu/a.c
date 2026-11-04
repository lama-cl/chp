/*
 * p69_nested_data_field_alias_to_inner_struct_same_tu/a.c
 *
 * Diagnostic for aliases into the middle of a nested field path.
 *
 * bp aliases &ctx.box, so consume_box(bp) should map the callee-relative
 * b->payload behavior onto run::ctx.box.payload and then back to run::x.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void run(void *x) {
  struct Context ctx;
  ctx.box.payload = x;

  struct Box *bp = &ctx.box;
  consume_box(bp);
}

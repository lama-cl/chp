/*
 * p66_data_field_struct_pointer_fnptr_alias_same_tu/a.c
 *
 * Same-TU ordinary function-pointer data-field behavior through a local
 * struct-pointer alias.  bp aliases &box, so cb(bp) should still map the
 * callee's b->payload behavior onto run::box.payload and run::x.
 */

struct Box {
  void *payload;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void run(void *x) {
  struct Box box;
  box.payload = x;
  struct Box *bp = &box;
  void (*cb)(struct Box *) = consume_box;
  cb(bp);
}

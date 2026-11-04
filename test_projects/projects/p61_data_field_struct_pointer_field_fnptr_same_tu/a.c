/*
 * p61_data_field_struct_pointer_field_fnptr_same_tu/a.c
 *
 * Same-TU field-function-pointer data-field behavior.  The callee's
 * relative b->payload cast/use should project through ops->consume(&box)
 * onto run::box.payload and the stored origin run::x.
 */

struct Box {
  void *payload;
};

struct Ops {
  void (*consume)(struct Box *);
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void run(struct Ops *ops, void *x) {
  struct Box box;
  box.payload = x;
  ops->consume = consume_box;
  ops->consume(&box);
}

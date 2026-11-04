/*
 * p59_data_field_struct_pointer_field_fnptr_cross_tu/a.c
 *
 * Cross-TU field-function-pointer version of p55.  The imported target consumes
 * a struct Box * and dereferences b->payload.  Import mode should project the
 * relative data-field behavior through ops->consume(&box).
 */

struct Box {
  void *payload;
};

struct Ops {
  void (*consume)(struct Box *);
};

extern void consume_box(struct Box *b);

void run(struct Ops *ops, void *x) {
  struct Box box;
  box.payload = x;
  ops->consume = consume_box;
  ops->consume(&box);
}

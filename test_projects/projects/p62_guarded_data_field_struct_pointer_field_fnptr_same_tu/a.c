/*
 * p62_guarded_data_field_struct_pointer_field_fnptr_same_tu/a.c
 *
 * Guarded same-TU field-function-pointer data-field behavior.  Branch guards
 * on the callback target should be preserved when projecting b->payload
 * behavior to run::box.payload and run::x.
 */

struct Box {
  void *payload;
};

struct Ops {
  void (*consume)(struct Box *);
};

void consume_int_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void consume_char_box(struct Box *b) {
  char *p = b->payload;
  *p += 1;
}

void run(struct Ops *ops, void *x, int tag) {
  struct Box box;
  box.payload = x;
  if (tag)
    ops->consume = consume_int_box;
  else
    ops->consume = consume_char_box;
  ops->consume(&box);
}

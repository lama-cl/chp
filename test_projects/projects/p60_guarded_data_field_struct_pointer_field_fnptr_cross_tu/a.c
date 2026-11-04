/*
 * p60_guarded_data_field_struct_pointer_field_fnptr_cross_tu/a.c
 *
 * Guarded cross-TU field-function-pointer data-field behavior.  The guarded
 * target set should project relative b->payload operations back to run::x with
 * branch provenance.
 */

struct Box {
  void *payload;
};

struct Ops {
  void (*consume)(struct Box *);
};

extern void consume_int_box(struct Box *b);
extern void consume_char_box(struct Box *b);

void run(struct Ops *ops, void *x, int tag) {
  struct Box box;
  box.payload = x;
  if (tag)
    ops->consume = consume_int_box;
  else
    ops->consume = consume_char_box;
  ops->consume(&box);
}

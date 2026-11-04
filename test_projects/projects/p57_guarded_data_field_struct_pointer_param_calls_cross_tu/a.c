/*
 * p57_guarded_data_field_struct_pointer_param_calls_cross_tu/a.c
 *
 * Guarded cross-TU data-field calls.  The operations imported from the two
 * callees should remain branch-separated on run::box.payload and run::x.
 */

struct Box {
  void *payload;
};

extern void consume_int_box(struct Box *b);
extern void consume_char_box(struct Box *b);

void run(void *x, int tag) {
  struct Box box;
  box.payload = x;

  if (tag)
    consume_int_box(&box);
  else
    consume_char_box(&box);
}

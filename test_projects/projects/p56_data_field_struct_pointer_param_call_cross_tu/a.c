/*
 * p56_data_field_struct_pointer_param_call_cross_tu/a.c
 *
 * Cross-TU version of p53.  consume_box() forwards b->payload to another
 * callee in b.c; the imported data-field summaries should still reach
 * run::box.payload and run::x.
 */

struct Box {
  void *payload;
};

extern void consume_box(struct Box *b);

void run(void *x) {
  struct Box box;
  box.payload = x;
  consume_box(&box);
}

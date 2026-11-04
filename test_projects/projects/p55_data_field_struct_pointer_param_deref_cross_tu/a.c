/*
 * p55_data_field_struct_pointer_param_deref_cross_tu/a.c
 *
 * Cross-TU version of p52.  consume_box() is defined in b.c and
 * dereferences b->payload as int *.  Import mode should project the relative
 * data-field behavior back to run::box.payload and to the value stored there
 * (run::x).
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

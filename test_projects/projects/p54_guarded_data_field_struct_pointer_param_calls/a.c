/*
 * p54_guarded_data_field_struct_pointer_param_calls/a.c
 *
 * C6d1: guarded same-TU calls to struct-pointer callees should preserve branch
 * provenance when their data-field pointer operations are transferred back to
 * the caller's field and stored origin.
 */

struct Box {
  void *payload;
};

void consume_int_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void consume_char_box(struct Box *b) {
  char *p = b->payload;
  *p += 1;
}

void run(void *x, int tag) {
  struct Box box;
  box.payload = x;

  if (tag)
    consume_int_box(&box);
  else
    consume_char_box(&box);
}

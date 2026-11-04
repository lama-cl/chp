/*
 * p53_data_field_struct_pointer_param_call_same_tu/a.c
 *
 * C6d1: the same-TU struct-pointer callee forwards a data field to another
 * same-TU callee.  Pointer operations from the inner callee should still be
 * summarized as uses of the struct-pointer parameter's field and transferred
 * back to the caller's stored origin.
 */

struct Box {
  void *payload;
};

void consume(void *v) {
  char *p = v;
  *p += 1;
}

void consume_box(struct Box *b) {
  consume(b->payload);
}

void run(void *x) {
  struct Box box;
  box.payload = x;
  consume_box(&box);
}

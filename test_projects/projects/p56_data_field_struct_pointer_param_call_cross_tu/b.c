/* p56_data_field_struct_pointer_param_call_cross_tu/b.c */

struct Box {
  void *payload;
};

static void consume_int(void *x) {
  int *p = x;
  *p += 1;
}

void consume_box(struct Box *b) {
  consume_int(b->payload);
}

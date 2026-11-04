/* p57_guarded_data_field_struct_pointer_param_calls_cross_tu/b.c */

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

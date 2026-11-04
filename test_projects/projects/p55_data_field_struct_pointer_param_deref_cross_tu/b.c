/* p55_data_field_struct_pointer_param_deref_cross_tu/b.c */

struct Box {
  void *payload;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

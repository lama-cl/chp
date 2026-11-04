/* p71_nested_data_field_alias_to_inner_struct_cross_tu/b.c */

struct Box {
  void *payload;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

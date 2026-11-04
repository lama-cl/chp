/* p58_data_field_struct_pointer_fnptr_cross_tu/b.c */

struct Box {
  void *payload;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

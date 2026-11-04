/*
 * p58_data_field_struct_pointer_fnptr_cross_tu/a.c
 *
 * Cross-TU function-pointer version of p55.  The imported target consumes a
 * struct Box * and dereferences b->payload.  Import mode should project the
 * relative data-field behavior through cb(&box).
 */

struct Box {
  void *payload;
};

extern void consume_box(struct Box *b);

void run(void *x) {
  struct Box box;
  void (*cb)(struct Box *) = consume_box;
  box.payload = x;
  cb(&box);
}

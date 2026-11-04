/*
 * p65_guarded_data_field_struct_pointer_fnptr_same_tu/a.c
 *
 * Guarded same-TU ordinary function-pointer data-field behavior.  The target
 * selection guard should remain attached when relative b->payload summaries are
 * projected through cb(&box).
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
  void (*cb)(struct Box *);
  if (tag)
    cb = consume_int_box;
  else
    cb = consume_char_box;
  cb(&box);
}

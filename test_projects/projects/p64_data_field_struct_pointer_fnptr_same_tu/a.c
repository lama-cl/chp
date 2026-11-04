/*
 * p64_data_field_struct_pointer_fnptr_same_tu/a.c
 *
 * Same-TU ordinary function-pointer data-field behavior.  The callee's
 * relative b->payload cast/use should project through cb(&box) onto
 * run::box.payload and the stored origin run::x.
 */

struct Box {
  void *payload;
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void run(void *x) {
  struct Box box;
  box.payload = x;
  void (*cb)(struct Box *) = consume_box;
  cb(&box);
}

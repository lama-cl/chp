/*
 * p63_data_field_local_ops_dot_field_fnptr_same_tu/a.c
 *
 * Same-TU field-function-pointer data-field behavior using a local Ops object
 * and dot-field call syntax instead of an Ops pointer and arrow syntax.
 */

struct Box {
  void *payload;
};

struct Ops {
  void (*consume)(struct Box *);
};

void consume_box(struct Box *b) {
  int *p = b->payload;
  *p += 1;
}

void run(void *x) {
  struct Box box;
  struct Ops ops;
  box.payload = x;
  ops.consume = consume_box;
  ops.consume(&box);
}

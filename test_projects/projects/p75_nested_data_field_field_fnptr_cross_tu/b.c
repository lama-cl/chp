/* p75_nested_data_field_field_fnptr_cross_tu/b.c */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

void consume_ctx(struct Context *c) {
  int *p = c->box.payload;
  *p += 1;
}

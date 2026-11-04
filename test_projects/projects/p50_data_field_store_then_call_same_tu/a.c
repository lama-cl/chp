/*
 * p50_data_field_store_then_call_same_tu/a.c
 *
 * C6d: a void* parameter is stored in a data field, then passed from that
 * field to a same-TU callee whose parameter is dereferenced as int*.
 */

struct Box {
  void *payload;
};

void consume(void *v) {
  int *p = v;
  *p += 1;
}

void run(void *x) {
  struct Box box;
  box.payload = x;
  consume(box.payload);
}

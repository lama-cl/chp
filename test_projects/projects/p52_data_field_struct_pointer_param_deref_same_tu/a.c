/*
 * p52_data_field_struct_pointer_param_deref_same_tu/a.c
 *
 * C6d1: a caller stores a void* parameter into a data field, then passes the
 * containing object by pointer to a same-TU callee.  The callee loads the field
 * through its struct-pointer parameter and dereferences it as int*.  The field
 * operation should be materialized on the caller field and projected back to
 * the original stored parameter.
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
  consume_box(&box);
}

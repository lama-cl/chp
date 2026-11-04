/*
 * p67_nested_data_field_store_then_deref_same_tu/a.c
 *
 * Diagnostic for C6d nested data-field support.
 *
 * The data pointer is stored in a nested field path ctx.box.payload and then
 * loaded/dereferenced through the same nested field path in the same TU.
 */

struct Box {
  void *payload;
};

struct Context {
  struct Box box;
};

void run(void *x) {
  struct Context ctx;
  ctx.box.payload = x;

  int *p = ctx.box.payload;
  *p += 1;
}

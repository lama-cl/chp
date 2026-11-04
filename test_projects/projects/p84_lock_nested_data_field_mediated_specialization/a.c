/*
 * p84_lock_nested_data_field_mediated_specialization/a.c
 *
 * C6f1 classification-lock test: nested data-field-mediated specialization.
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

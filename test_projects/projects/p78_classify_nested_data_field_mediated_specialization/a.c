/*
 * p78_classify_nested_data_field_mediated_specialization/a.c
 *
 * void * is stored in a nested data-field path and later used concretely.
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

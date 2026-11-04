/*
 * p77_classify_data_field_mediated_specialization/a.c
 *
 * void * is stored in a data field and later used concretely through that field.
 */

struct Box {
  void *payload;
};

void run(void *x) {
  struct Box box;
  box.payload = x;

  int *p = box.payload;
  *p += 1;
}

/*
 * p83_lock_data_field_mediated_specialization/a.c
 *
 * C6f1 classification-lock test: data-field-mediated concrete specialization.
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

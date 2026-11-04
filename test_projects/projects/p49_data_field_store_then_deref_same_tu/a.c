/*
 * p49_data_field_store_then_deref_same_tu/a.c
 *
 * C6d: a void* parameter is stored in a data field, then later loaded from
 * that field and dereferenced as int*.  Pointer-operation facts should be
 * reported on the field and propagated back to the original parameter with
 * data-field provenance.
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

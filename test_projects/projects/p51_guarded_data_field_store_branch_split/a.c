/*
 * p51_guarded_data_field_store_branch_split/a.c
 *
 * C6d: guarded stores to the same data field should preserve branch-specific
 * origins when a later field load is dereferenced.
 */

struct Box {
  void *payload;
};

void run(void *a, void *b, int tag) {
  struct Box box;

  if (tag)
    box.payload = a;
  else
    box.payload = b;

  int *p = box.payload;
  *p += 1;
}

/*
 * p85_lock_same_tu_fnptr_callback_specialization/a.c
 *
 * C6f1 classification-lock test: resolved same-TU function-pointer callback.
 */

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void run(void *x) {
  void (*cb)(void *) = handle_int;
  cb(x);
}

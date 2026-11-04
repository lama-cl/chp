/*
 * p86_lock_guarded_fnptr_callback_specialization/a.c
 *
 * C6f1 classification-lock test: guarded callback alternatives.
 */

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void handle_char(void *x) {
  char *p = x;
  *p += 1;
}

void run(void *x, int tag) {
  void (*cb)(void *) = 0;
  if (tag)
    cb = handle_int;
  else
    cb = handle_char;

  cb(x);
}

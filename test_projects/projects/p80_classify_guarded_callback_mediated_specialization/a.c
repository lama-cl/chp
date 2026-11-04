/*
 * p80_classify_guarded_callback_mediated_specialization/a.c
 *
 * The callback target is selected by a guard, producing branch-structured
 * concrete alternatives for the same void * parameter.
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

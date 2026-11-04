/*
 * p79_classify_callback_mediated_specialization/a.c
 *
 * void * is interpreted concretely by a known local callback target.
 */

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void run(void *x) {
  void (*cb)(void *) = handle_int;
  cb(x);
}

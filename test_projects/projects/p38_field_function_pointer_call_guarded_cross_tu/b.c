/* p38_field_function_pointer_call_guarded_cross_tu/b.c */

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

void handle_char(void *x) {
  char *p = x;
  *p += 1;
}

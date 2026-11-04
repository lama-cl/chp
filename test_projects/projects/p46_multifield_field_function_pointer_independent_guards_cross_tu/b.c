/* p46_multifield_field_function_pointer_independent_guards_cross_tu/b.c */

void parse_int(void *x) {
  int *p = x;
  *p += 1;
}

void parse_char(void *x) {
  char *p = x;
  *p += 1;
}

void emit_double(void *x) {
  double *p = x;
  *p += 1.0;
}

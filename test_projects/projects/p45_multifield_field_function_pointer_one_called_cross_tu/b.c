/* p45_multifield_field_function_pointer_one_called_cross_tu/b.c */

void parse_int(void *x) {
  int *p = x;
  *p += 1;
}

void emit_char(void *x) {
  char *p = x;
  *p += 1;
}

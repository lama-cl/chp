/* p37_field_function_pointer_call_import_cross_tu/b.c */

void handle_int(void *x) {
  int *p = x;
  *p += 1;
}

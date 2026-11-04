// typeatom-t4-text-key-alignment.c
//
// Purpose:
//   Mixed structured TypeAtom paths and legacy rendered-text fallback should
//   use the same metric key space. Counts should not exceed the displayed type
//   sets merely because some observations came from text fallback.
//
// Expected intent:
//   distinctTypes/pointerTypes/nonVoidPointerTypes should align with the
//   displayed sets for simple {void*, char*, int*} histories.

void text_key_alignment(void *x) {
  char *pc = x;
  int *pi = x;
  (void)pc;
  (void)pi;
}

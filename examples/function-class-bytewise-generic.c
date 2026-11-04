// function-class-bytewise-generic.c
//
// Purpose:
//   Function-level classification for bytewise generic pointer manipulation.
//
// Expected function behavior classification:
//   advance_bytes
//     class: BytewiseGenericFunction
//
// Rationale:
//   The void* parameter is converted to char* for byte-level pointer movement,
//   but there is no typed dereference/write evidence.
void *advance_bytes(void *base, int n) {
  char *p = base;
  p = p + n;
  return p;
}

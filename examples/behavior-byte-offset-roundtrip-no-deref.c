// behavior-byte-offset-roundtrip-no-deref.c
//
// Purpose:
//   Common C byte-offset idiom: T* -> char* -> ptrarith(char*) -> T*.
//   This should not be treated the same as byte-view to an unrelated concrete
//   type.
//
// Expected behavior:
//   p
//     behavior: ByteOffsetRoundTripNoDeref
//
// Rationale:
//   The pointer temporarily becomes char* for byte-addressing and returns to
//   the same concrete pointer type.  No dereference is performed.
void demo(int *p, int n) {
  char *c = (char *)p + n * sizeof(int);
  int *q = (int *)c;
}

// behavior-byte-offset-different-concrete-no-deref.c
//
// Purpose:
//   Byte offset followed by recovery to a different concrete pointer type.
//
// Expected behavior:
//   p
//     behavior: ByteOffsetToDifferentConcreteNoDeref
//
// Rationale:
//   This is stronger than same-type byte-offset round trip.
void demo(int *p, int n) {
  char *c = (char *)p + n;
  double *q = (double *)c;
}

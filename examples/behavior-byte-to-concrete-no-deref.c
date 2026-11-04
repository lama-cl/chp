// behavior-byte-to-concrete-no-deref.c
//
// Purpose:
//   Direction matters.  char* => double* is not merely a byte view.
//   It is a concrete pointer recovered from a byte view.
//
// Expected direct/storage histories:
//   pc : char * => double *
//
// Expected behavior:
//   pc
//     behavior: ByteViewToConcreteNoDeref
//
// Rationale:
//   No dereference happens, so the suffix remains NoDeref.  But the direction
//   is byte-view to concrete, not concrete to byte-view.
void demo(char *pc) {
  double *pd = (double *)pc;
}

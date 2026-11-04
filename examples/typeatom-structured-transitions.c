// typeatom-structured-transitions.c
//
// Purpose:
//   Regression test for structured TypeAtom transition classification.
//
// The history includes normal array-to-pointer decay and pointer arithmetic.
// It should not set intToPtr=yes merely because the rendered array type contains
// the string "int".
//
// Expected key history:
//   arr : int[4] => int * => ptrarith(int *) => void * => char *
//
// Expected metric cues:
//   intToPtr=no
//   ptrToInt=no
//   risk should not be DangerousProvenance
void demo(int i) {
  int arr[4];
  void *v = &arr[i];
  char *pc = v;
}

//
// Behavior classifier expectation:
//   v : void * => char * should be ByteViewNoDeref, not ConcreteSpecialized,
//   because char* is a byte-oriented view and there is no dereference evidence.

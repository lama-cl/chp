// typeatom-array-vs-branch.c
//
// Purpose:
//   Regression test for structured TypeAtom classification.
//
//   Array syntax such as int[10] must not be confused with branch syntax.
//   Array-to-pointer decay int[10] => int * should not be treated as
//   dangerous integer-to-pointer provenance.
//
// Expected key history:
//   arr : int[10] => int * => ptrarith(int *) => void * => char *
//
// Expected metric cues for arr:
//   shape should not be BranchPolymorphic
//   risk should not be DangerousProvenance
//
// This is similar to array-derived-array-object.c but is kept as a focused
// TypeAtom regression test.
void demo(int i) {
  int arr[10];
  void *v = &arr[i];
  char *pc = v;
}

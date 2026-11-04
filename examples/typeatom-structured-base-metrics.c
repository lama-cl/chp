// typeatom-structured-base-metrics.c
//
// Purpose:
//   T3 regression: direct/storage history component APIs should carry a
//   structured base TypeAtom into metric computation.
//
// This makes base types such as int[8] classified using Clang type structure
// rather than by parsing the printed base text.
//
// Expected key history:
//   arr : int[8] => int * => ptrarith(int *) => void * => char *
//
// Expected metric cues for arr:
//   intToPtr=no
//   ptrToInt=no
//   risk should not be DangerousProvenance
//   shape should not be BranchPolymorphic
void demo(int i) {
  int arr[8];
  void *v = &arr[i];
  char *pc = v;
}

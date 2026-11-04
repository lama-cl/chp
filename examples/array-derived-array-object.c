// array-derived-array-object.c
//
// Purpose:
//   Address-taking from an array object should be treated as a derived pointer
//   from the array storage.
//
// Program pattern:
//   int arr[10]
//     -> void *v = &arr[i]
//     -> char *pc = v
//
// Expected direct cast histories:
//   arr : int[10] => int * => ptrarith(int *) => void * => char *
//   v   : void * => char *
//
// Depending on Clang's exact array-to-pointer decay representation, the printed
// base for arr may vary slightly, but the key expected segment is:
//   ptrarith(int *) => void * => char *
//
// Expected metric cues for arr:
//   ptrArith=yes
//   ptrArithTypes: {int *}
void demo(int i) {
  int arr[10];
  void *v = &arr[i];
  char *pc = v;
}

//
// Observation-role expectation:
//   The intermediate projection from:
//     void *v = &arr[i];
//   should be marked/treated as Intermediate. If later extended by:
//     char *pc = v;
//   the final summary for arr should not contain a spurious Parallel arm.
//
// Preferred final direct/storage history for arr:
//   arr : int[10] => int * => ptrarith(int *) => void * => char *

//
// Regression note:
//   Clang may present the array subscript base as an ArrayToPointerDecay
//   ImplicitCastExpr. The analysis should inspect the underlying DeclRefExpr
//   type to detect arr's array type and insert:
//     int * => ptrarith(int *)
//   consistently in both the intermediate and extended observations.
//   Without this, the final history may incorrectly show:
//     ⟨| ptrarith(int *) => void *,
//        int * => ptrarith(int *) => void * => char * |⟩

//
// Metric expectation:
//   arr's base type int[10] must not be interpreted as a branch merely because
//   it contains brackets. The expected shape should be ByteView or StableViaVoid
//   depending on the rest of the history, but not BranchPolymorphic.
//   The array-to-pointer step int[10] => int * is normal array decay and should
//   not be classified as DangerousProvenance.

// function-class-tagged-union-like.c
//
// Purpose:
//   Function-level classification for branch-guarded type alternatives.
//
// Expected function behavior classification:
//   dispatch_by_tag
//     class: TaggedUnionLikeFunction
//
// Rationale:
//   The same void* parameter is interpreted as different concrete pointer
//   types under tag-like branches.
void dispatch_by_tag(void *x, int tag) {
  if (tag == 1) {
    int *pi = x;
  } else {
    double *pd = x;
  }
}

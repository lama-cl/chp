// branch-position-deep.c
//
// Purpose:
//   Branch-cast metrics should show that the branch split happens after a
//   deeper sequential prefix.
//
// Expected branch metrics:
//   branchPosition should be deeper than branch-position-basic.c because the
//   value first goes through void* and ptrarith(char*) before branching.
//   branchAltSeqLen should reflect one-step branch alternatives.
//
// This example is synthetic but useful for checking branch-depth accounting.
void demo(void *x, int tag, int n) {
  int *pi = x;
  char *base = (char *)pi + n;

  if (tag == 1) {
    int *p1 = (int *)base;
    *p1 += 1;
  } else {
    double *p2 = (double *)base;
    *p2 += 1.0;
  }
}

//
// Provenance-aware subsumption expectation:
//   The intermediate projection
//     int * => char * => ptrarith(char *)
//   from:
//     char *base = (char *)pi + n;
//   should not remain as a separate parallel arm once both guarded branches
//   extend it to int* and double* alternatives.

//
// Factored branch-metrics expectation:
//   Even if the printed history is:
//     x : void * => ⟨[tag == 1] int * => char * => ptrarith(char *) => int *,
//                    [!(tag == 1)] int * => char * => ptrarith(char *) => double *⟩
//
//   branch metrics should be computed as if the common prefix were factored:
//     x : void * => int * => char * => ptrarith(char *)
//              => ⟨[tag == 1] int *, [!(tag == 1)] double *⟩
//
//   Expected metric cues:
//     branchPosition: earliest=4, latest=4, max=4
//     branchAltSeqLen: min=1, max=1, avg=1.00
//     branchNonVoidTypes should emphasize branch-specific alternatives:
//       {double *, int *}
//   The common prefix types int*/char* are still present in the overall
//   cast-history metrics, but branch-specific metrics should not count them
//   as alternative-specific branch choices.

//
// View-completeness expectation:
//   x and demo.$0 should be FullView because deref-readwrite evidence is
//   projected to the root/function parameter.
//   pi/base may be PartialView because they carry intermediate cast/branch
//   structure but do not necessarily receive all downstream operation evidence.
//   base should be BranchTypeSplitNoDeref rather than ByteViewReadOnly when it
//   has branch-specific type alternatives but no direct dereference evidence.

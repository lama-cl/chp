// ptrarith-derived-var.c
//
// Purpose:
//   Pointer arithmetic in a declaration initializer should produce a derived
//   origin for the newly declared variable.
//
// Program pattern:
//   int *pidv
//     -> int *pnew = pidv + d / 2
//     -> char *pc = (char *)pnew
//
// Expected key flow/cast events:
//   pidv : int * => ptrarith(int *)
//   pnew : int * => ptrarith(int *) => char *
//
// Expected direct cast histories:
//   pidv : int * => ptrarith(int *) => char *
//   pnew : int * => ptrarith(int *) => char *
//
// Expected storage-location cast histories:
//   pidv : int * => ptrarith(int *) => char *
//   pnew : int * => ptrarith(int *) => char *
//
// Expected metrics cues for pidv/pnew:
//   ptrarith >= 1
//   ptrArith=yes
//   ptrArithTypes: {int *}
//
// Note:
//   This example checks binary arithmetic where the integer offset expression is
//   nontrivial:
//     d / 2
//   The arithmetic detector only needs Clang's expression types; it does not
//   need to evaluate the integer expression.
void demo(int *pidv, int d) {
  int *pnew = pidv + d / 2;
  char *pc = (char *)pnew;
}

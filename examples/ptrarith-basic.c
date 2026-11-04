// ptrarith-basic.c
//
// Purpose:
//   Binary pointer arithmetic should preserve the root origin and insert an
//   explicit ptrarith(T *) step into the cast history.
//
// Program pattern:
//   void *x
//     -> int *pidv = x
//     -> pidv = pidv + 4
//     -> char *pci = (char *)pidv
//
// Expected key flow/cast events:
//   x    : void * => int *
//   pidv = pidv + 4 updates the reaching origin to include ptrarith(int *)
//   pidv : void * => int * => ptrarith(int *) => char *
//
// Expected direct cast histories:
//   x    : void * => int * => ptrarith(int *) => char *
//   pidv : int * => char *
//     The standalone ptrarith(int *) arm from pidv = pidv + 4 should NOT appear
//     as an independent parallel arm; the ptrarith step is used as attachment
//     context for later observations.
//
// Expected storage-location cast histories:
//   x    : void * => int * => ptrarith(int *) => char *
//   pidv : void * => int * => ptrarith(int *) => char *
//
// Expected metrics cues for x/pidv:
//   ptrarith >= 1
//   ptrArith=yes
//   ptrArithTypes: {int *}
//
// Note:
//   This example checks assignment-form binary arithmetic:
//     pidv = pidv + 4;
//   Compound updates such as pidv += 4 and pidv++ are intentionally covered
//   by a later pointer-arithmetic patch.
void demo(void *x) {
  int *pidv = x;
  pidv = pidv + 4;
  char *pci = (char *)pidv;
}

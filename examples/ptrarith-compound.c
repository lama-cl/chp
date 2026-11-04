// ptrarith-compound.c
//
// Purpose:
//   Compound pointer arithmetic updates should mutate the pointer's reaching
//   origin/path without erasing it.
//
// Program pattern:
//   void *x
//     -> int *p = x
//     -> p += n
//     -> char *pc = (char *)p
//
// Expected direct cast histories:
//   x : void * => int * => ptrarith(int *) => char *
//   p : int * => ptrarith(int *) => char *
//
// Expected metric cues:
//   ptrArith=yes
//   ptrArithTypes: {int *}
void demo(void *x, int n) {
  int *p = x;
  p += n;
  char *pc = (char *)p;
}

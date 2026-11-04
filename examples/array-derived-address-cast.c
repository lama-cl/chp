// array-derived-address-cast.c
//
// Purpose:
//   If a cast is applied to an array-derived address, the cast should appear
//   after ptrarith(T *).
//
// Program pattern:
//   int *p
//     -> char *pc = (char *)&p[i]
//
// Expected direct cast histories:
//   p : int * => ptrarith(int *) => char *
//
// Expected function summary:
//   demo($0 : int * => ptrarith(int *) => char *, $1 : int)
//
// Expected metric cues:
//   ptrArith=yes
//   ptrArithTypes: {int *}
void demo(int *p, int i) {
  char *pc = (char *)&p[i];
}

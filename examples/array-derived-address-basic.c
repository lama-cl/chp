// array-derived-address-basic.c
//
// Purpose:
//   Address-taking of an array-derived element should preserve the origin and
//   insert ptrarith(T *).
//
// Program pattern:
//   int *p
//     -> int *q = &p[i]
//     -> char *pc = (char *)q
//
// Expected direct cast histories:
//   p : int * => ptrarith(int *) => char *
//   q : int * => ptrarith(int *) => char *
//
// Expected storage-location cast histories:
//   p : int * => ptrarith(int *) => char *
//   q : int * => ptrarith(int *) => char *
//
// Expected metric cues:
//   ptrArith=yes
//   ptrArithTypes: {int *}
//
// Important:
//   &p[i] should not be counted as an array-element dereference/read.
//   It is address computation / pointer arithmetic.
void demo(int *p, int i) {
  int *q = &p[i];
  char *pc = (char *)q;
}

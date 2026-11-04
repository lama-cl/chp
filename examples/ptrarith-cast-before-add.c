// ptrarith-cast-before-add.c
//
// Purpose:
//   When a cast occurs before pointer arithmetic, the history should preserve
//   the order:
//     int * => char * => ptrarith(char *)
//
// Program pattern:
//   void *x
//     -> int *pidv = x
//     -> char *pc = (char *)pidv + sizeof(int) * 3
//
// Expected key flow/cast events:
//   x    : void * => int *
//   pidv : void * => int * => char * => ptrarith(char *)
//
// Expected direct cast histories:
//   x    : void * => int * => char * => ptrarith(char *)
//   pidv : int * => char * => ptrarith(char *)
//
// Expected storage-location cast histories:
//   x    : void * => int * => char * => ptrarith(char *)
//   pidv : void * => int * => char * => ptrarith(char *)
//
// Expected metrics cues for x/pidv:
//   ptrarith >= 1
//   ptrArith=yes
//   ptrArithTypes: {char *}
//
// Important distinction:
//   This should NOT be normalized to:
//     int * => ptrarith(int *) => char *
//   because the arithmetic is performed after casting to char *.
void demo(void *x) {
  int *pidv = x;
  char *pc = (char *)pidv + sizeof(int) * 3;
}

// ptrarith-incdec.c
//
// Purpose:
//   Unary pointer increment/decrement should update the pointer's current
//   origin/path.
//
// Program pattern:
//   void *x
//     -> int *p = x
//     -> p++
//     -> --p
//     -> char *pc = (char *)p
//
// Expected direct cast histories:
//   x : void * => int * => ptrarith(int *) => ptrarith(int *) => char *
//   p : int * => ptrarith(int *) => ptrarith(int *) => char *
//
// Note:
//   This first version records each inc/dec as one ptrarith(T *) step. Later
//   normalization may choose to summarize repeated adjacent ptrarith steps.
void demo(void *x) {
  int *p = x;
  p++;
  --p;
  char *pc = (char *)p;
}

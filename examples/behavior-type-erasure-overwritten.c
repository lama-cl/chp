// behavior-type-erasure-overwritten.c
//
// Purpose:
//   Strong update through a void* variable should not make the overwritten
//   source appear concrete-specialized.
//
// Program:
//   pi1 is only assigned into pv and then overwritten.
//   pj1 is the reaching value later viewed as char*.
//
// Expected:
//   pi1 : int * => void *
//     behavior: TypeErasedToVoidNoDeref
//
//   pj1 : int * => void * => char *
//     behavior: ByteViewNoDeref
void demo(void) {
  int i = 0, j = 0;
  int *pi1 = &i;
  int *pj1 = &j;
  void *pv = pi1;
  pv = pj1;
  char *pc1 = pv;
}

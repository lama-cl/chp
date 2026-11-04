// ptrptr-write-basic.c
//
// Purpose:
//   Lightweight pointer-to-pointer storage write.
//
// Pattern:
//   int i;
//   int *pi = &i;
//   void *pv;
//   void **pp = &pv;
//   *pp = pi;
//   char *pc = pv;
//
// Expected key cast histories:
//   pi : int * => void * => char *
//   pv : int * => void * => char *
//
// Explanation:
//   void **pp = &pv establishes pp as a pointer to pv's storage.
//   *pp = pi should strongly update pv's stored origin to pi.
//   char *pc = pv then extends pi/pv with => char *.
void demo(void) {
  int i = 0;
  int *pi = &i;
  void *pv;
  void **pp = &pv;
  *pp = pi;
  char *pc = pv;
}

//
// Behavior-metric expectation after classifier polish:
//   - A concrete one-type reinterpretation such as void * => int * with no
//     dereference should be ConcreteSpecialized, not OpaqueForwarding.
//   - A char*/byte view with no dereference should be ByteViewNoDeref, not
//     ByteViewReadOnly, because no memory read necessarily happened.

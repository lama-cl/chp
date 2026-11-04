// ptrptr-read-basic.c
//
// Purpose:
//   Lightweight pointer-to-pointer storage read.
//
// Pattern:
//   void *pv;
//   void **pp = &pv;
//   int *pi = (int *)*pp;
//
// Expected key cast histories:
//   pv : void * => int *
//   pp : void **
//
// Expected pointer/storage behavior:
//   *pp should be resolved as a read of pv's storage.
//   The int* observation should be recorded on pv, not only on pp.
//
// Limitations:
//   This tests only local variable storage targets. Field storage and nested
//   pointer-to-pointer chains are deferred.
void demo(void *pv) {
  void **pp = &pv;
  int *pi = (int *)*pp;
}

//
// Behavior-metric expectation after classifier polish:
//   - A concrete one-type reinterpretation such as void * => int * with no
//     dereference should be ConcreteSpecialized, not OpaqueForwarding.
//   - A char*/byte view with no dereference should be ByteViewNoDeref, not
//     ByteViewReadOnly, because no memory read necessarily happened.

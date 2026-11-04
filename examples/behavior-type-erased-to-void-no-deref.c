// behavior-type-erased-to-void-no-deref.c
//
// Purpose:
//   T* => void* is type erasure into an opaque interface, not concrete
//   specialization.
//
// Expected direct history:
//   pi1 : int * => void *
//
// Expected behavior:
//   pi1
//     behavior: TypeErasedToVoidNoDeref
//     directionalPatterns: ... typeErasureToVoid=yes,
//                            concreteSpecializationFromVoid=no ...
void demo(void) {
  int i = 0;
  int *pi1 = &i;
  void *pv = pi1;
}

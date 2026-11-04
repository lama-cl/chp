// behavior-byte-view-no-deref.c
//
// Purpose:
//   A single char*/byte view with no dereference should be ByteViewNoDeref,
//   not ConcreteSpecialized.
//
// Program:
//   int arr[4];
//   void *v = &arr[i];
//   char *pc = v;
//
// Expected key local history:
//   v : void * => char *
//
// Expected behavior metric for v:
//   view: LocalView
//   behavior: ByteViewNoDeref
//
// Rationale:
//   char* is used only as a byte-oriented view/conversion. There is no
//   dereference/read/write through pc in this example.
void demo(int i) {
  int arr[4];
  void *v = &arr[i];
  char *pc = v;
}

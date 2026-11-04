// ptrptr-read-after-write.c
//
// Purpose:
//   Combined pointer-to-pointer storage write followed by read-through-*pp.
//
// Pattern:
//   *pp = pi;
//   char *pc = (char *)*pp;
//
// Expected key cast histories:
//   pi : int * => void * => char *
//   pv : int * => void * => char *
//
// The later read through *pp should resolve to pv's current stored origin,
// which was strongly updated by *pp = pi.
void demo(void) {
  int i = 0;
  int *pi = &i;
  void *pv;
  void **pp = &pv;
  *pp = pi;
  char *pc = (char *)*pp;
}

//
// Regression expectation:
//   The store through *pp creates an intermediate projection:
//     pi : int * => void *
//   The later read/cast through *pp extends the same path:
//     pi : int * => void * => char *
//   Therefore the final summary for pi should not keep a spurious Parallel:
//     pi : int * => ⟨| void *, void * => char * |⟩
//   Preferred:
//     pi : int * => void * => char *

//
// Behavior-metric expectation after classifier polish:
//   - A concrete one-type reinterpretation such as void * => int * with no
//     dereference should be ConcreteSpecialized, not OpaqueForwarding.
//   - A char*/byte view with no dereference should be ByteViewNoDeref, not
//     ByteViewReadOnly, because no memory read necessarily happened.

// function-class-reinterpreting-write-concrete.c
//
// Purpose:
//   A void* parameter written through a concrete specialized type should be in
//   the same high-level function class as byte-view write, with different
//   evidence flags.
//
// Expected parameter behavior:
//   idv.$0 : void *
//     behavior: ConcreteSpecializedWrite
//
// Expected function behavior:
//   idv
//     class: ReinterpretingWriteFunction
//     evidenceFlags: ... reinterpretingWrite=yes, byteWrite=no, concreteWrite=yes ...
void *idv(void *x) {
  int *pidv = x;
  *pidv = *pidv + 1;
  return pidv;
}

// function-class-reinterpreting-write-byte.c
//
// Purpose:
//   A void* parameter written through char* is a reinterpreting write function.
//
// Expected parameter behavior:
//   f.$0 : void *
//     behavior: ByteViewWrite
//
// Expected function behavior:
//   f
//     class: ReinterpretingWriteFunction
//     evidenceFlags: ... reinterpretingWrite=yes, byteWrite=yes, concreteWrite=no ...
void f(void *pvf) {
  char *pc = pvf;
  *pc += 1;
}

// merged-history-behavior-metrics.c
//
// Purpose:
//   The default metrics output should show both history-only structure and
//   operation-aware behavior in one section.
//
// Expected key output:
//
// Pointer behavior metrics:
//   pvf
//     view: FullView
//     historyShape: StableViaVoid
//     historyRisk: Low
//     behavior: ByteViewWrite
//     ...
//     history:
//       ...
//     operations:
//       useKinds: byteView=yes, byteRead=no, byteWrite=yes, ...
//
// Rationale:
//   The cast history itself is simple: void* => char*.
//   The behavior is stronger because the pointer is written through char*.
void f(void *pvf) {
  char *pc = pvf;
  *pc += 1;
}

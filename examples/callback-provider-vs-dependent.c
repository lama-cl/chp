// callback-provider-vs-dependent.c
//
// Purpose:
//   Distinguish the higher-order function parameter that provides behavior from
//   the data pointer whose cast history depends on that callback.
//
// Program pattern:
//   ho.$0 is a function-pointer callback provider.
//   ho.$1 is a void* data pointer passed through ho.$0 after a char* view.
//
// Expected function parameter behavior metrics:
//   ho.$0 : void *(*)(void *)
//     paramRole: CallbackBehaviorProvider
//     behavior: NotWildLike
//     wildLike: no
//
//   ho.$1 : void *
//     paramRole: CallbackDependentDataPointer
//     behavior: CallbackDependentByteViewPointer
//     summary: char * => void * => $0.$0
//
// Rationale:
//   ho.$0 determines behavior, but ho.$1 is the data pointer whose future
//   interpretation depends on the callback.
void *idv(void *x) {
  int *pi = x;
  *pi += 1;
  return pi;
}

void ho(void *(*pf)(void *), void *pvho) {
  char *pcho = pvho;
  pf(pcho);
}

void demo(void) {
  int i = 0;
  int *pi = &i;
  void *pv = pi;
  ho(idv, pv);
}

//
// Refined behavior expectation:
//   Because ho.$1 is first viewed as char* and then passed to the callback,
//   the behavior should be CallbackDependentByteViewPointer and the output
//   should include:
//     callbackInputView: byteView(char *) before callback

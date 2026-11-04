// callback-dependent-byteview.c
//
// Purpose:
//   A data pointer passed to a callback after being viewed as char* should be
//   classified more specifically than generic CallbackDependentPointer.
//
// Expected behavior:
//   pvho
//     behavior: CallbackDependentByteViewPointer
//     callbackInputView: byteView(char *) before callback
//
//   ho.$1 : void *
//     paramRole: CallbackDependentDataPointer
//     behavior: CallbackDependentByteViewPointer
//     summary: char * => void * => $0.$0
void *idv(void *x) {
  int *pi = x;
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

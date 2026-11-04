// typeatom-t4-function-pointer-signature.c
//
// Purpose:
//   T4 serializes function-pointer TypeAtoms with signature information.
//   Function-pointer parameters should remain CallbackBehaviorProvider rather
//   than being mistaken for ordinary void* data pointers.

typedef void *(*cb_t)(void *);

void higher(cb_t cb, void *x) {
  cb(x);
}

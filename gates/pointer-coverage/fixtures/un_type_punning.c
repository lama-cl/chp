// GATE un-type-punning
// Untagged union used to reinterpret a pointer without any cast expression.
// No CastExpr exists in the AST, yet the pointer changes static type.
//
// Probe subjects:
//   pun_to_int.$0 (p : void *)  -- written as u.vp, read as u.ip: must record
//                                  a union-mediated reinterpretation
//                                  void * => int *
//   pun_to_bits.$0 (p : int *)  -- written as u.ip, read as u.bits: must
//                                  record a pointer-to-integer view
#include <stdint.h>

union ptr_pun {
  void *vp;
  int *ip;
  float *fp;
  uintptr_t bits;
};

int pun_to_int(void *p) {
  union ptr_pun u;
  u.vp = p;
  int *pi = u.ip;
  return *pi;
}

uintptr_t pun_to_bits(int *p) {
  union ptr_pun u;
  u.ip = p;
  return u.bits;
}

float pun_chain(int *p) {
  union ptr_pun u;
  u.ip = p;
  float *pf = u.fp;
  return *pf;
}

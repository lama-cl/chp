// behavior-void-to-concrete-specialized.c
//
// Purpose:
//   void* => T* remains concrete specialization.
//
// Expected:
//   pv : void * => int *
//     behavior: ConcreteSpecialized
//     directionalPatterns: ... concreteSpecializationFromVoid=yes ...
void demo(void *pv) {
  int *pi = pv;
}

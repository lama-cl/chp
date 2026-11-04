// function-class-concrete-specializer.c
//
// Purpose:
//   Function-level classification for a function that specializes void* to one
//   concrete type.
//
// Expected function behavior classification:
//   specialize_int
//     class: ConcreteSpecializerFunction
//
// Rationale:
//   The void* parameter is interpreted as int*, but there is no multi-target,
//   branch, callback, or unsafe write evidence.
void specialize_int(void *x) {
  int *p = x;
}

// function-class-opaque-polymorphic.c
//
// Purpose:
//   Function-level classification for an opaque polymorphic function.
//
// Expected function behavior classification:
//   id_opaque
//     class: OpaquePolymorphicFunction
//     evidence:
//       $0 behavior=OpaqueForwarding or TypeErasedToVoidNoDeref
//
// Rationale:
//   The void* parameter is returned/stored opaquely and not specialized to a
//   concrete non-byte type.
void *id_opaque(void *x) {
  return x;
}

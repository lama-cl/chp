// fptr-24-indirect-call-return-alias.c
//
// Fact F24: data flowing *back* out of an indirect call.  The target returns
// its parameter; the caller views the result as int*.  With a direct call
// (int *q = id(ctx)) the return-alias continuation already attributes the int*
// use to ctx; through a resolved function pointer it is dropped today.
//
// Real world: allocator hooks that return void *, e.g. Lua's
//   return (*g->frealloc)(g->ud, block, os, ns);
// (test_projects/bench-3c/lua-5.4.1/src/lmem.c:35).
//
// @fptr-fact F24 indirect-call-return-alias
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
static void *id(void *p) { return p; }

int run(void *ctx) {
  void *(*f)(void *) = id;
  int *q = f(ctx);
  return *q;
}

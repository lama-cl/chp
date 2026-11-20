// fptr-13-fptr-through-void-pointer.c
//
// Fact F13: a function pointer is converted to void * and back (POSIX permits
// this; ISO C does not guarantee it).  The caller passes use_int as void *, the
// callee converts it to handler_t and calls it.  The higher-order binding is
// lost at the void * boundary today: caller::ctx is reported OpaqueForwarding
// even though its target is known.
//
// Real world: dlsym() returns void * that is cast to a function pointer
// (test_projects/bench-3c/lua-5.4.1/src/loadlib.c:141); generic containers
// storing callbacks in void * slots.
//
// @fptr-fact F13 fptr-through-void-pointer
// @subject   caller::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static int run(void *opaque_fn, void *ctx) {
  handler_t cb = (handler_t)opaque_fn;
  return cb(ctx);
}

int caller(void *ctx) { return run((void *)use_int, ctx); }

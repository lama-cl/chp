// fptr-12-out-param-pointer-to-fptr.c
//
// Fact F12: a function pointer is produced through an out-parameter of type
// handler_t * (a pointer to a function pointer).  Requires a summary fact
// "get writes {use_int} into *$0" and its application to &cb at the call.
//
// @fptr-fact F12 out-param-pointer-to-fptr
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static void get(handler_t *out) { *out = use_int; }

int run(void *ctx) {
  handler_t cb;
  get(&cb);
  return cb(ctx);
}

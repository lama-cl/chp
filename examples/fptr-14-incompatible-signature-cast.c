// fptr-14-incompatible-signature-cast.c
//
// Fact F14: a function is called through a function-pointer type with a
// different signature (C17 6.3.2.3p8: calling through an incompatible type is
// undefined behaviour).  The target is resolved today and long* evidence is
// transferred, but nothing records that the void* -> long* view arose from a
// signature-laundering cast rather than a cast expression in the source.
//
// Real world: GLib's G_CALLBACK() casts every handler to void (*)(void); the
// same pattern appears with `(void (*)(int))interrupted` signal handlers
// (test_projects/bench-3c/thttpd-2.29/extras/htpasswd.c:159).
//
// @fptr-fact F14 incompatible-signature-cast
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=long*
// @expected  category=MonomorphicActiveSpecialization types=long* resolution=exact@L0 sigmismatch=true
typedef int (*handler_t)(void *);

static int use_long(long *p) { return (int)*p; }

int run(void *ctx) {
  handler_t cb = (handler_t)use_long;
  return cb(ctx);
}

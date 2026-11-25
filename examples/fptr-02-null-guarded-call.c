// fptr-02-null-guarded-call.c
//
// Fact F02 (control): the ubiquitous `if (cb) cb(arg)` idiom.  The null check
// must not hide the known target, and the null alternative must not be counted
// as a second target.
//
// Real world: optional hooks such as icecast format_plugin_t::apply_settings
// (test_projects/bench-3c/icecast-2.4.4/src/format.h:54) are null-checked
// before every call.
//
// @fptr-fact F02 null-guarded-call
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

int run(int disabled, void *ctx) {
  handler_t cb = use_int;
  if (disabled)
    cb = 0;
  if (cb)
    return cb(ctx);
  return 0;
}

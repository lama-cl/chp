// fptr-03-global-initializer.c
//
// Fact F03 (control, supported today *by accident of traversal order*): a
// file-scope function pointer with a constant initializer and no other stores,
// called from another function.  It resolves only because the initializer is
// visited before run() and the per-function save/restore of L0 state keeps it.
// See F34 (definition after use: unresolved) and F35 (reassigned elsewhere:
// unsound).  After the extension, globals resolve through L1 facts (D3).
//
// Real world: parson's allocator hook
//   static JSON_Malloc_Function parson_malloc = malloc;
// (test_projects/bench-3c/parson/parson.c:62).
//
// @fptr-fact F03 global-initializer
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static handler_t g_handler = use_int;

int run(void *ctx) { return g_handler(ctx); }

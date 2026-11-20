// fptr-04-global-registry-assigned-elsewhere.c
//
// Fact F04: a global function pointer is assigned in one function (the
// registration / "set hook" API) and called in another.  L0 state is saved and
// restored per function, so run() never sees the store in set_handler().
//
// Real world: parson json_set_allocation_functions() stores the user hook into
// the global parson_malloc (test_projects/bench-3c/parson/parson.c:2052), which
// is called from every allocation site.
//
// Required: L1 flow-insensitive storage facts for globals: g_handler may hold
// {use_int} (the zero initializer contributes no target).
//
// @fptr-fact F04 global-registry-assigned-elsewhere
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static handler_t g_handler;

void set_handler(void) { g_handler = use_int; }

int run(void *ctx) { return g_handler(ctx); }

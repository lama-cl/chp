// fptr-32-constructor-attribute-registration.c
//
// Fact F32: registration performed by a GNU constructor function that runs
// before main.  No call edge reaches install(); only L1 flow-insensitive
// global facts can see the store.  (Linker-section tables such as Linux
// initcalls are the same idea and are documented, not fixtured.)
//
// @fptr-fact F32 constructor-attribute-registration
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static handler_t g_handler;

__attribute__((constructor)) static void install(void) { g_handler = use_int; }

int run(void *ctx) { return g_handler(ctx); }

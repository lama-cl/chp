// fptr-35-global-reassigned-elsewhere-unsound.c
//
// Fact F35 (SOUNDNESS BUG today): a global initialized with use_int and
// reassigned to use_dbl by another function.  The file-scope initializer is
// treated as the flow-sensitive *current* value inside run(), so run::ctx is
// reported as a monomorphic int* specialization and use_dbl is lost.
//
// Required (D3): inside a function, a global with no local store on the path
// is read through its L1 flow-insensitive slot = initializer ∪ all stores in
// the TU/project = {use_int, use_dbl}.  The selection is temporal (which store
// ran last), not a value visible at the call, so by D8 the set is unguarded.
//
// Real world: parson's parson_malloc = malloc (parson.c:62) is replaced by
// json_set_allocation_functions (parson.c:2052).
//
// @fptr-fact F35 global-reassigned-elsewhere-unsound
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
// @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

static handler_t g_handler = use_int;

int run(void *ctx) { return g_handler(ctx); }

void reconfigure(void) { g_handler = use_dbl; }

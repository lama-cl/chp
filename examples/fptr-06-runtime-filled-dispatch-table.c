// fptr-06-runtime-filled-dispatch-table.c
//
// Fact F06: a mutable function-pointer array filled by element stores in an
// init function and indexed in another function.  Needs array-element storage
// facts (field-insensitive per array: table[*]).
//
// Real world: the LIL interpreter fills its callback table at run time,
//   lil->callback[cb] = proc;
// (test_projects/bench-genc2rust/lil/main.c:871) and later reads and casts
// entries, e.g. (lil_setvar_callback_proc_t)lil->callback[LIL_CALLBACK_SETVAR]
// (main.c:501).
//
// @fptr-fact F06 runtime-filled-dispatch-table
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_int_neg(void *p) { int *q = p; return -*q; }

static handler_t table[2];

void init_table(void) {
  table[0] = use_int;
  table[1] = use_int_neg;
}

int run(unsigned op, void *ctx) { return table[op & 1](ctx); }

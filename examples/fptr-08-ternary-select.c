// fptr-08-ternary-select.c
//
// Fact F08: a function pointer chosen by `?:`.  resolveFunctionValue() has no
// ConditionalOperator case, so even identical arms are unresolved today.  The
// two targets read different types so the guard must be carried:
// cond ? use_int : use_dbl  ->  ⟨[cond] int*, [!cond] double*⟩.
//
// @fptr-fact F08 ternary-select
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=GuardedActiveReinterpretation types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

int run(int is_int, void *ctx) {
  handler_t cb = is_int ? use_int : use_dbl;
  return cb(ctx);
}

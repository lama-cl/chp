// fptr-09-switch-select-unsound.c
//
// Fact F09 (SOUNDNESS BUG today): a function pointer assigned in switch arms.
// TraverseSwitchStmt walks the body syntactically, so the last assignment
// (use_dbl) strongly overwrites the first and run::ctx is reported as a
// monomorphic double* specialization: the int* target is silently lost.
//
// Required: switch arms join fptr target state like if/else, with case guards.
//
// Real world: state/command selection by `switch (opcode)` that assigns a
// handler, then calls it once after the switch.
//
// @fptr-fact F09 switch-select-unsound
// @subject   run::ctx
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double*
// @expected  category=GuardedActiveReinterpretation types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

int run(int kind, void *ctx) {
  handler_t cb = 0;
  switch (kind) {
  case 0:
    cb = use_int;
    break;
  default:
    cb = use_dbl;
    break;
  }
  return cb(ctx);
}

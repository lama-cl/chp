// fptr-05-const-dispatch-table.c
//
// Fact F05: a constant array of function pointers indexed by a runtime value
// (opcode / command dispatch).  The call callee is an ArraySubscriptExpr, which
// resolveFunctionValue() does not handle today.
//
// Both entries specialize to int*, so the only fact under test is that the
// table's element target set {use_int, use_int_neg} is found.  Heterogeneous
// tables are F33.
//
// Real world: Lua library registration arrays, e.g.
//   static const luaL_Reg loadedlibs[] = {...}
// (test_projects/bench-3c/lua-5.4.1/src/linit.c:42).
//
// @fptr-fact F05 const-dispatch-table
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_int_neg(void *p) { int *q = p; return -*q; }

static const handler_t table[] = { use_int, use_int_neg };

int run(unsigned op, void *ctx) { return table[op & 1](ctx); }

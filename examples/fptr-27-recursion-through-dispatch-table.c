// fptr-27-recursion-through-dispatch-table.c
//
// Fact F27: recursion closed only through an indirect call.  op_rec calls
// ops[...] which may be op_rec itself; the named-call SCC must include the
// indirect edge so the summary of op_rec reaches the fixpoint {int*}.
//
// Real world: interpreters whose calls re-enter through function pointers,
// e.g. Lua invoking a C function with  n = (*f)(L);
// (test_projects/bench-3c/lua-5.4.1/src/ldo.c:481), which may call back into
// the interpreter.
//
// @fptr-fact F27 recursion-through-dispatch-table
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
typedef int (*op_fn)(int, void *);

static int op_leaf(int d, void *p) { int *q = p; return *q + d; }
static int op_rec(int d, void *p);

static const op_fn ops[2] = { op_leaf, op_rec };

static int op_rec(int d, void *p) { return ops[d > 0](d - 1, p); }

int run(void *ctx) { return op_rec(3, ctx); }

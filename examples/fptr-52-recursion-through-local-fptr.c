// fptr-52-recursion-through-local-fptr.c
//
// Fact F52 (recursion; round 4): recursion closed through a resolved local
// fptr variable.  Today such a call creates no NamedCallEdge, so walk is an
// acyclic SCC (07 §4); seam S9 adds the edge.  The subject p is advanced by
// one char per level, so its history needs the recursive fixpoint.
//
// Today (measured): ByteOrientedActiveAccess char*,int* (types are real history), but resolution=unresolved@-: the recursive call through f is not composed and is no SCC edge.
//
// @fptr-fact F52 recursion-through-local-fptr
// @subject   run::ctx
// @baseline  category=ByteOrientedActiveAccess callback=ResolvedCallback behavior=ConcreteSpecializedRead types=char*,int* resolution=unresolved@- sigmismatch=false
// @expected  types=char*,int* resolution=exact@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
typedef int (*walk_fn)(int, void *);
static int walk(int d, void *p) {
  walk_fn f = walk;
  if (d > 0)
    return f(d - 1, (char *)p + 1);
  int *q = p;
  return *q;
}
int run(void *ctx) { return walk(3, ctx); }

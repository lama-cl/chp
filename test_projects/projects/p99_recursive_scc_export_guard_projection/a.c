// C6g4p1 regression: pointer-use facts flow from a recursive callee summary
// back to an acyclic caller.  The runtime edges wrapper -> f are `into-scc`,
// but the source summary belongs to the f/g recursive SCC and must use only
// the leaf-local guard for fixed-point membership.
static void g(void *x, int depth);

static void f(void *x, int depth) {
  if (depth <= 0)
    return;
  char *p = x;
  if (depth & 1)
    *p += 1;
  g(x, depth - 1);
}

static void g(void *x, int depth) {
  if (depth <= 0)
    return;
  f(x, depth - 1);
}

void wrapper(void *x, int mode) {
  if (mode)
    f(x, 3);
  else
    f(x, 2);
}

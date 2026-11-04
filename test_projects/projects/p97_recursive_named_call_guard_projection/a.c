// C6g4p regression: two caller-context guards reach one unguarded leaf use
// through a recursive same-TU SCC.  The solver should retain the leaf's
// unconditional guard for membership and keep the mode / !mode contexts only
// as bounded witnesses.
static void g(void *x, int n, int mode);

void f(void *x, int n, int mode) {
  if (n <= 0)
    return;
  if (mode)
    g(x, n - 1, mode);
  else
    g(x, n - 1, mode);
}

static void g(void *x, int n, int mode) {
  if (n <= 0)
    return;
  char *p = x;
  *p += 1;
  f(x, n - 1, mode);
}

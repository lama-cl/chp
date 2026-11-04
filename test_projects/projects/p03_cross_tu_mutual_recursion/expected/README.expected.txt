Purpose:
  Cross-TU mutual recursion.

Important functions:
  a.c: f(void *x) calls g(x)
  b.c: g(void *y) calls f(y)

Expected C1/local behavior:
  f and g both contain unresolved/importable cross-TU direct-call refs.

Expected C2/C3a behavior:
  The project call graph should contain:
    f -> g
    g -> f
  This forms one recursive SCC: {f, g}.

Expected safe behavior before full C4:
  Do not inline indefinitely.
  Keep bounded recursive placeholder or diagnostic:
    cross-tu-recursive-scc: {f, g}

Expected later C4 behavior:
  Solve the SCC by fixpoint with widening/iteration limit.

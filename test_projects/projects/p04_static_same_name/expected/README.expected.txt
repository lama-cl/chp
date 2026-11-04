Purpose:
  Static/internal same-name functions.

Important functions:
  a.c: static helper(void *x) uses int*
  b.c: static helper(void *x) uses char*

Expected C2 index behavior:
  These helpers have the same name and same apparent signature, but they are
  source-scoped internal functions. They should not be reported as external
  duplicate definitions.

Expected status:
  internal-same-name-signature or source-scoped
  resolution: source-scoped

Expected later C3 behavior:
  Calls from fa should resolve to a.c's helper.
  Calls from fb should resolve to b.c's helper.

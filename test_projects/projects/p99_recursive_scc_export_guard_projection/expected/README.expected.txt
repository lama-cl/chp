C6g4p1 regression target: recursive-SCC source-summary guard projection.

Call graph:
  wrapper -> f (under mode)
  wrapper -> f (under !mode)
  f -> g
  g -> f

Runtime relation for wrapper -> f:
  into-scc (caller wrapper is acyclic; callee f belongs to a recursive SCC).

Expected for wrapper::$0:
  - pointer-use facts imported from f/g use the leaf-local guard `(depth & 1)`
    for same-TU named-call solver membership;
  - the caller contexts `mode` and `!mode` are retained only as bounded
    recursive guard witnesses, not as distinct solver states;
  - --analysis-progress reports a nonzero
    named-call-recursive-source-guard-projected count when the transfer is
    exercised;
  - entering a recursive SCC alone does not synthesize recursive evidence;
    existing structured recursive evidence remains the reporting authority.

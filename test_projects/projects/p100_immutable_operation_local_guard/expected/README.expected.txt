C6g4r regression target: immutable source-level operation-local guards.

Inside relay_a(), `selected` has two reaching alternatives from the branches
`choose` and `!choose`.  The leaf operation `*bytes += 1` is lexically
unconditional after the join.

Expected pointer-use separation:
  - records rooted at relay_a::x retain full route guards containing `choose`
    or `!choose` where appropriate;
  - the same records serialize operationLocalGuard as JSON null / <always>,
    rather than copying the reaching-origin guard;
  - the recursive named-call solver can therefore merge those alternatives
    when recursive-SCC guard projection uses operationLocalGuard;
  - genuine local guards on separately guarded leaf operations remain intact.

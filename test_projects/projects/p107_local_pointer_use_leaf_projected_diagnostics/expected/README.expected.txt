C6g4x1 regression target: local pointer-use diagnostics grouped by immutable
leaf identity rather than by full local-storage membership.

Run with --analysis-progress.

At the `consume(cursor)` leaf, the reaching alternatives originating at `input`
may differ in their effective guard and in the number of same-type
`ptrarith(char *)` atoms.  Required diagnostic behavior:

  1. Records for the same source entity, immutable operation site, kind, type,
     lexical operation-local guard, and canonical leaf detail share one
     leaf-projected diagnostic group even when their reaching guard differs.
  2. The active diagnostic exposes `effective-guard-only`,
     `attachment-only`, or `attachment-and-effective-guard` according to the
     best retained comparable variant.  The exact class depends on the
     underlying origin representation, but it must no longer say
     `comparable-existing-records=0` for every replay.
  3. A repeated same-type pointer-arithmetic run is retained as diagnostic
     evidence only.  C6g4x1 does not normalize, suppress, or merge it.
  4. This project is intentionally small and is not a proof that the local
     clib-original loop is repaired; it protects the diagnostic partition that
     C6g4x accidentally made route-sensitive.

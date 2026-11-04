C6g4x1 regression target: diagnostic-only local pointer-use attachment growth
classification.

Run with --analysis-progress.  C6g4x1 observes only successfully inserted,
local AST pointer-use records whose attachment contains ptrarith(...).

Required behavior:
  1. The diagnostic group is a leaf projection.  It retains entity/canonical
     field path, immutable operation site, kind, pointer/access type, lexical
     operation-local guard and its origin, and canonical leaf detail.  It
     deliberately excludes attachment, effective/reaching guard, mutable
     reporting span, and provenance; those are reported as bounded variants.
  2. The progress line exposes aggregate local attachment groups, insertions,
     new attachment variants, ptrarith-only variants, attachment-only,
     effective-guard-only, attachment-and-effective-guard, provenance-only,
     transfer-span-only, and other membership variants, bounded-example
     truncation, and consecutive same-AST-node insertions.
  3. The active diagnostic records the raw and normalized candidate attachment,
     pointer-arithmetic token sequence/count, comparable existing record count,
     a membership-difference vector, a classified difference class, and one
     best comparable existing variant including effective guard, reporting span,
     and provenance.
  4. The three cursor arithmetic statements in this fixture have distinct
     operation sites.  C6g4x1 must report them separately rather than treating
     ordinary distinct source arithmetic as a cycle.
  5. This patch is observational only.  It changes neither PointerUseRecord
     equality nor pointer-arithmetic cycle suppression, eager membership,
     named-call membership, summaries, JSON, or text results.

C6g4v regression target: finite eager-transfer membership with explicit
specialization retained and route-only context moved into bounded witnesses.

Required behavior:
  1. The direct_wrapper(value) calls in the two complementary branches have
     the same target entity, immutable leaf operation site, local leaf guard,
     attachment/type/access/use-kind, and specialization provenance.  Their
     differing effective guards, full `via ...` chains, and transfer spans must
     occupy one eager local pointer-use membership record, with alternate route
     evidence counted as a bounded C6g4v witness.
  2. The two consume_box(&first) paths likewise merge for
     first.payload.  The consume_box(&second) materialization remains distinct
     because the outer target entity/canonical field path is different.
  3. The two invoke(callback, value) paths merge only for the same resolved
     callback target.  invoke(callback_other, value) remains distinct because
     selected target/source operation identity differs.
  4. Full diagnostic storage-key counts may exceed projected-membership-key
     counts.  Projected detail removes named-call and data-field relay clauses,
     but retains the final leaf operation; with --analysis-progress, C6g4v
     reports both and increments eager-pointer-use-route-projected plus bounded
     witness counters.
  5. The existing C6g4u active-transfer report must print <pending> for exit
     and target-growth fields while a transfer is still active; it must print
     numeric final values only after completion.

Scope boundary:
  - Same-TU eager direct-param, data-field-param, and resolved higher-order
    transfers use C6g4v projected local membership.
  - AST-local insertion, cross-TU import materialization, named-call solver
    membership, operationSiteSpan semantics, recursive guard projection, and
    higher-order target resolution remain otherwise unchanged.

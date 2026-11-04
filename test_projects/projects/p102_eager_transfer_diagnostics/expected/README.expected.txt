C6g4u regression target: diagnostics for eager pointer-use materialization
that occurs during AST traversal, before pointer-use-summary-attempts starts.

With --analysis-progress, C6g4u must emit scoped diagnostics for:
  1. direct-param: direct_wrapper(value) importing leaf's $0 use summary;
  2. data-field-param: consume_box(&box) materializing box.payload uses;
  3. higher-order-direct: invoke(callback, value) specializing callback.$0.

For each active/complete eager transfer, the log must identify:
  - source-records-entry / visited / exit;
  - candidates, inserted, and deduplicated local pointer-use records;
  - target entity count and target-record growth;
  - source operation groups keyed by immutable source-op-loc;
  - candidate groups and local-storage membership differences, including
    transfer-span, attachment, effective guard, detail, and provenance.

Scope boundary:
  - Diagnostics only.  C6g4u does not change recordPointerUse equality,
    operationSiteSpan semantics, named-call membership, guard projection,
    summary serialization, or project-index behavior.

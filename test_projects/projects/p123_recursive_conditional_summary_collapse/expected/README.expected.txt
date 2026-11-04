C6g5d1 regression target: edge-local widening for a self-recursive call
encountered during if-condition traversal.

Required properties:

  - recursive_condition_summary.$0 -> recursive_condition_summary.$0 is
    reported as recorded-in-if-condition=yes;
  - the function summary retains one readable
      rec{same-tu-if-scc|...}
    terminal marker;
  - named-call-recursive-condition-tokens-introduced is nonzero;
  - named-call-recursive-condition-transfers-suppressed is nonzero after the
    replayed self edge;
  - the SCC worklist converges; and
  - the marker is local summary evidence: it is not propagated to
    run_recursive_condition_summary through ordinary named-call transfer.

Scope boundary:

  The condition is still visited.  C6g5d1 widens only the matching
  same-SCC condition edge before reading/instantiating its evolving callee
  summary.

C6g5b regression target: named-call combined-domain SCC-local worklist accounting.

Required behavior:
  1. With --analysis-progress, the completed same-TU named-call solve emits
     phase=named-call-round-complete and reports:
       - named-call-round-param-observation-changes,
       - named-call-round-pointer-use-insertions,
       - named-call-round-recursive-evidence-upgrades,
       - named-call-round-observation-snapshot-changed,
       - named-call-round-pointer-use-snapshot-changed,
       - named-call-round-combined-snapshot-changed, and
       - named-call-round-disposition.
  2. The solver reports
       named-call-scheduler=scc-local-parameter-dependency-worklist
     together with SCC component counts, local worklist drains,
     local-reenqueue, cross-component-signal, pending-work, and
     edge-execution-budget counters.
  3. The mutually recursive parse_a/parse_b SCC is drained locally until both
     parameter cast histories and parameter pointer-use summaries are stable.
     Pointer-use quiescence alone must not discard a pending dependency caused
     by a parameter-history update.
  4. The entry caller is activated only after the parse_a/parse_b component
     completes.  The final text report prints scheduler, SCC component details,
     edge-execution count, termination reason, and the two solved domains.
     Normal completion uses termination=scc-worklist-quiescent.  A nonempty
     local queue at the safety bound is reported as
     scc-worklist-execution-limit.

Scope boundary:
  - C6g5b changes same-TU scheduling only; it keeps the existing transfer
    helpers and frozen directParamSummaries/directUseSummaries bases.
  - It does not change cross-TU SCC scheduling, field sensitivity,
    higher-order specialization, local/eager membership, or witness bounds.

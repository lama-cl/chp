C6g5c regression target: watchdog diagnostics for a recursive named-call edge
recorded while TraverseIfStmt is visiting its condition.

Run with deterministic watchdog triggering:

  --named-call-watchdog-diagnostics
  --named-call-watchdog-filter=watchdog_recursive
  --named-call-watchdog-threshold-ms=0
  --named-call-watchdog-max-events=16

Required stderr properties:

  - contains at least one [named-call-watchdog] record with
    event=slow-edge-complete;
  - caller and callee are watchdog_recursive;
  - recursive-edge=yes;
  - recorded-in-if-condition=yes;
  - if-condition-depth-at-call is at least 1;
  - records summary-observation-normalization, equality, and render timing
    fields plus normalization/equality/render counters; and
  - ordinary cast histories, pointer-use summaries, SCC scheduling, and
    convergence are unchanged whenever watchdog diagnostics are disabled.

Scope boundary:
  - the patch is diagnostic only.  It does not suppress condition traversal,
    widen or truncate histories, change summary membership, impose a timeout,
    or abort a transfer.

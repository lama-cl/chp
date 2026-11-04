C6g5b regression target: same-TU SCC-local parameter-dependency worklist.

The source is intentionally ordered so that the entry call and all three relay
calls are visited before leaf_cast_and_use's local cast summary is finalized.
The SCC scheduler must nevertheless activate components in callee-first order
and propagate the unsigned-char interpretation through:

  leaf_cast_and_use.$0 -> relay_one.$0 -> relay_two.$0
      -> relay_three.$0 -> exercise_named_call_dependency_worklist.$0

Required semantic result:

  leaf_cast_and_use.$0, relay_one.$0, relay_two.$0, relay_three.$0,
  and exercise_named_call_dependency_worklist.$0 all include the
  void * => unsigned char * interpretation.

With --analysis-progress, the same-TU solver must report:

  named-call-scheduler=scc-local-parameter-dependency-worklist
  named-call-scc-components=7
  named-call-scc-recursive-components=0
  named-call-worklist-initial-seed-edges=6
  named-call-worklist-dependency-reenqueues=0
  named-call-scc-cross-component-dependency-signals=3
  named-call-worklist-edge-executions=6
  named-call-round-disposition="scc-worklist-quiescent"

The two unrelated entry branches (unrelated_int and unrelated_long) are still
seeded exactly once.  Unlike C6g5a, the primary chain no longer needs three
cross-component re-enqueues: every component waits for its callees and then
executes its owned edge(s) once.

Scope boundary:
  - This test concerns only same-TU named direct-call SCC scheduling.
  - Cross-TU SCC scheduling, field-function-pointer replay, higher-order
    specialization, and the C6g4z4b immutable call-site snapshots are unchanged.

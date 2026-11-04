C6g5b regression target: local draining of a recursive same-TU named-call SCC.

The direct-call graph contains these components:

  leaf_cast_and_use                         (acyclic leaf)
  { scc_a, scc_b }                          (one recursive SCC)
  unrelated_int                             (acyclic leaf)
  exercise_named_call_scc_local_worklist   (acyclic entry)

Required semantic result:

  leaf_cast_and_use.$0, scc_a.$0, scc_b.$0, and
  exercise_named_call_scc_local_worklist.$0 all include the
  void * => unsigned char * interpretation.

With --analysis-progress / text output:

  - named-call-scheduler=scc-local-parameter-dependency-worklist;
  - exactly one recursive SCC is present, with scc_a and scc_b as members;
  - the recursive component is drained locally: it has internal edges and at
    least one local dependent re-enqueue or coalesced local re-enqueue while
    the leaf result is circulated through the cycle;
  - the entry component is activated only after that recursive component is
    quiescent; and
  - unrelated_int remains an acyclic component and is not part of the cycle.

Scope boundary:
  - The fixture exercises same-TU SCC scheduling and existing recursive
    transfer widening only.
  - It does not exercise cross-TU SCC import, field/function-pointer replay,
    or higher-order specialization.

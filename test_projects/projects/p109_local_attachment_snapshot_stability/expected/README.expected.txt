C6g4x1b regression target: snapshot-safe local attachment diagnostics

Run with --analysis-progress.  The alternatives reaching consume(cursor) may
produce several pointer-arithmetic attachment variants in one leaf-projected
diagnostic group.  The diagnostic must construct its progress snapshot from
bounded copies before appending a new variant to the group vector.

Required properties:
  1. No pointer or reference into group.variants is dereferenced after a
     potentially reallocating push_back.
  2. The progress line reports:
       local-pointer-use-attachment-diagnostic-snapshot-model=pre-retention-copy
     and does not disable the diagnostic with
       local-pointer-use-attachment-diagnostic-failure-stage=progress-snapshot
     merely because variant retention reallocates its vector.
  3. Pointer-use membership, solver state, emitted histories, and prevalence
     sidecars remain unchanged by this report-only fixture.

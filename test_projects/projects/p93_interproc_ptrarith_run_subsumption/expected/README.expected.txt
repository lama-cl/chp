C6g4m regression target: transfer-boundary pointer-arithmetic run subsumption.

Requirements:
  - distinct_local_steps_then_call retains the two local source-level
      ptrarith(char *)
    state transitions introduced by p += 1 and p += 2.  C6g4m does not alter
    the local C6g4l source-operation state.

  - The use imported from sink.$0 into distinct_local_steps_then_call.$0 is
    represented with one interprocedural ptrarith(char *) attachment run.
    Repeated same-type PtrArith atoms at this one call boundary are idempotent
    for pointer-use summary membership.

  - cast_boundary_is_not_collapsed retains the int* and char* distinction:
    the interprocedural normalization does not merge PtrArith steps across an
    AsType/cast boundary.

Performance requirement:
  - addPointerUseToFunctionParam must use the insertion boolean from
    addUniquePointerUseSummaryRecord rather than render all summary rows before
    and after every candidate.

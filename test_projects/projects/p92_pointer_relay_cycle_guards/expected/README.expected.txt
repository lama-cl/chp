C6g4l regression target: source-level pointer/data-field relay cycle guards.

Requirements:
  - distinct_pointer_arithmetic_sites must retain two distinct
      ptrarith(int *)
    steps before the later char * interpretation.  The guard must not act as a
    global ptrarith-depth cap or collapse different source statements.

  - distinct_data_field_uses contains two different source operations through
    box->payload.  Their field-local/root pointer-use evidence may be grouped
    for reporting, but the cycle guard must not suppress one merely because the
    field path, pointer kind, and type are the same.

Scope:
  - The guard is intended for cyclic/replayed transfer of an identical
    source-level operation on the same root + field path + guard context.
  - This regression does not require a precise record-count assertion because
    pointer-use rows retain operation/provenance reporting detail.

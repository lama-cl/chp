Expected C6c behavior:
  Field-sensitive function-pointer targets contain guarded alternatives:
    run::ops.process -> handle_int  [assignment] [guard=tag]
    run::ops.process -> handle_char [assignment] [guard=!(tag)]

  Field-sensitive call resolution contains same-TU guarded targets:
    run via ops->process -> handle_int  [same-tu-field-function-pointer-call] [guard=tag]
    run via ops->process -> handle_char [same-tu-field-function-pointer-call] [guard=!(tag)]

  C6c materializes the same-TU field-function-pointer target pointer-use
  summaries onto the caller actual run::x.  The entity-level Pointer use
  summaries / Grouped pointer-operation summaries for run::x should include:
    [tag]     origin=field-fnptr, target=handle_int,  field=run::ops.process, deref-readwrite as=int *,  access=int
    [!(tag)]  origin=field-fnptr, target=handle_char, field=run::ops.process, deref-readwrite as=char *, access=char

  This complements the existing function-parameter behavior metrics for run.$1;
  C6c does not add a new cast-history propagation path.

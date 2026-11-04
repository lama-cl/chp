C6d3 regression target: guarded cross-TU field-function-pointer import of relative data-field summaries.

In import mode for a.c, ops->consume(&box) should resolve both imported targets
and project their relative b->payload behavior onto run::box.payload and run::x
with branch guards preserved.

Expected core evidence:
  run::box.payload : void * => <branch alternatives for int * / char *>
  run::x           : void * => <branch alternatives for int * / char *>
  [tag] deref-readwrite as=int * from consume_int_box
  [!(tag)] deref-readwrite as=char * from consume_char_box
  pointer-use rows include origin=data-field, source=summary-import,
  edge=data-field-field-function-pointer-call, field/calleeExpr provenance for
  ops->consume, and target-specific provenance.

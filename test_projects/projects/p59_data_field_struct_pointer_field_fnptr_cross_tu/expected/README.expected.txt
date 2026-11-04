C6d3 regression target: cross-TU field-function-pointer import of relative data-field summaries.

In import mode for a.c, ops->consume(&box) should resolve to imported
consume_box and project consume_box::b.payload behavior onto:
  - run::box.payload
  - run::x, through the prior box.payload = x store

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run::x pointer-use rows include origin=data-field, source=summary-import,
  edge=data-field-field-function-pointer-call, field/calleeExpr provenance for
  ops->consume, target=consume_box, and deref-readwrite as=int *.

C6d4 regression target: same-TU field-function-pointer projection of relative data-field summaries.

For ops->consume(&box), the same-TU target consume_box has relative field behavior:
  consume_box::b.payload : void * => int *
  consume_box::b.payload deref-readwrite as=int *

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run::x receives deref-readwrite as=int * through run::box.payload

Pointer-use rows should preserve callback/data-field provenance, including:
  origin=data-field
  source=same-tu-data-field-field-fnptr
  edge=data-field-field-function-pointer-call
  calleeExpr=ops->consume

C6d4c additionally expects the caller function summary to be refreshed after
field-origin replay:
  run($0 : struct Ops *, $1 : void * => int *)

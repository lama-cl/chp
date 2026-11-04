C6d5 regression target: same-TU ordinary function-pointer projection of relative data-field summaries.

For cb(&box), cb resolves to local target consume_box.  The callee has relative field behavior:
  consume_box::b.payload : void * => int *
  consume_box::b.payload deref-readwrite as=int *

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run($0 : void * => int *)

Pointer-use rows should preserve ordinary function-pointer/data-field provenance, including:
  origin=data-field
  source=same-tu-data-field-fnptr
  edge=data-field-function-pointer-call
  calleeExpr=cb

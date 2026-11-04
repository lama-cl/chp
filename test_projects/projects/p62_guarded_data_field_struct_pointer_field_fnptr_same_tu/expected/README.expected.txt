C6d4 regression target: guarded same-TU field-function-pointer data-field projection.

Expected core evidence in run:
  [tag]     run::box.payload / run::x receives int * cast/use behavior
  [!(tag)]  run::box.payload / run::x receives char * cast/use behavior

Pointer-use rows should include branch-preserved deref-readwrite operations:
  [tag]     deref-readwrite as=int *
  [!(tag)]  deref-readwrite as=char *

The rows should identify the data-field callback edge:
  source=same-tu-data-field-field-fnptr
  edge=data-field-field-function-pointer-call
  calleeExpr=ops->consume

C6d4c additionally expects guarded field-FP replay to preserve target-selection guards in cast histories and summaries:
  run::x           : void * => ⟨[tag] int *, [!(tag)] char *⟩
  run::box.payload : void * => ⟨[tag] int *, [!(tag)] char *⟩
  run($1 : void * => ⟨[tag] int *, [!(tag)] char *⟩)
The result should not collapse into an unguarded parallel ⟨| int *, char * |⟩.

C6d5 regression target: guarded same-TU ordinary function-pointer projection of relative data-field summaries.

For cb(&box), cb resolves to a guarded local target set:
  [tag]     consume_int_box
  [!(tag)]  consume_char_box

Expected core evidence:
  run::box.payload : void * => <branch with [tag] int * and [!(tag)] char *>
  run::x           : void * => <branch with [tag] int * and [!(tag)] char *>
  run($0 : void * => <same branch history>, $1 : int)

The result should be branch-sensitive, not an unguarded Parallel:
  not: run::x : void * => <Parallel int *, char *>

Pointer-use rows should preserve ordinary function-pointer/data-field provenance:
  source=same-tu-data-field-fnptr
  edge=data-field-function-pointer-call
  calleeExpr=cb

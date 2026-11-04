C6d4 regression target: same-TU data-field projection through dot-form field-function-pointer call.

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run::x receives deref-readwrite as=int * through run::box.payload

The callback edge should be recognized even though the callee expression is
ops.consume rather than ops->consume.

C6d4c additionally expects the caller function summary to be refreshed after
dot-form field-FP data-field replay:
  run($0 : void * => int *)

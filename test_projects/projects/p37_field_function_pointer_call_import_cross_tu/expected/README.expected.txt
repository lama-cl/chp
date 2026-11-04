Expected C5b behavior:
  Field-sensitive function-pointer targets contains:
    run::ops.process -> handle_int [assignment]
  Cross-TU function-pointer summary lookup for a.c contains the field callee:
    calleeExpr = ops.process
    target = handle_int
    status = found
    paramImport = composed and/or pointerUseImport = composed
  run::x should receive imported pointer-use evidence from handle_int.$0:
    int * deref-readwrite

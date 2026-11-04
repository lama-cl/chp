Expected C5a behavior:
  Field-sensitive function-pointer targets contains:
    run::ops.process -> handle_int [assignment]
  JSON fieldFunctionPointerTargets contains the same fact.
  C5a should not yet resolve ops.process(x) as a call to handle_int.

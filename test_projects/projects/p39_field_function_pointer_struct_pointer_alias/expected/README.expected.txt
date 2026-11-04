Expected C5e signal:

Field-sensitive function-pointer targets:
  run::ops.process -> handle_int [assignment]

Field-sensitive function-pointer call resolution:
  run via p->process -> handle_int [same-tu-field-function-pointer-call] [composed]

run::x should receive handle_int's int* deref-readwrite evidence even though the call is through p->process.

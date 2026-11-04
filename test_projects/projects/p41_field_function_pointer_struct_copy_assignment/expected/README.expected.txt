Expected C5e signal:

Field-sensitive function-pointer targets:
  run::base.process -> handle_int [initializer]
  run::local.process -> handle_int [struct-copy-assignment]

Field-sensitive function-pointer call resolution:
  run via local.process -> handle_int [same-tu-field-function-pointer-call] [composed]

run::x should receive handle_int's int* deref-readwrite evidence.

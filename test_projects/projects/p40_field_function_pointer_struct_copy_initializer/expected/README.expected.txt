Expected C5e signal:

Field-sensitive function-pointer targets:
  base.process -> handle_char [initializer]
  run::local.process -> handle_char [struct-copy-initializer]

Field-sensitive function-pointer call resolution:
  run via local.process -> handle_char [same-tu-field-function-pointer-call] [composed]

run::x should receive handle_char's char* deref-readwrite evidence.

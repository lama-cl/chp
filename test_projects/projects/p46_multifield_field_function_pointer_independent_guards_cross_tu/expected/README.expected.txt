C5h0 expected behavior for p46:

- Target facts should be field-specific:
    run::ops.parse -> parse_int   [guard=tag]
    run::ops.parse -> parse_char  [guard=!(tag)]
    run::ops.emit  -> emit_double

- Call resolution should preserve field and argument separation:
    run via ops->parse -> parse_int   [guard=tag]
    run via ops->parse -> parse_char  [guard=!(tag)]
    run via ops->emit  -> emit_double

- run.$1 / x should receive only the guarded parse alternatives:
    run($1 : void * => ⟨[tag] parse_int⌜int *⌝parse_int,
                         [!(tag)] parse_char⌜char *⌝parse_char⟩)

- run.$2 / y should receive only emit_double:
    run($2 : void * => emit_double⌜double *⌝emit_double)

- Negative checks:
    emit_double must not appear in x's summary.
    parse_int/parse_char must not appear in y's summary.

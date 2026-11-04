C5h0 expected behavior for p45:

- Target facts should include both assignments:
    run::ops.parse -> parse_int
    run::ops.emit  -> emit_char

- Call resolution should include only:
    run via ops.parse -> parse_int

- run.$0 / run::x should include parse_int only:
    run($0 : void * => parse_int⌜int *⌝parse_int)

- Negative check:
    emit_char / char * must not appear in run.$0, run::x, pointer-use summaries
    for x, or call-site specialized observations, because ops.emit is never
    called.

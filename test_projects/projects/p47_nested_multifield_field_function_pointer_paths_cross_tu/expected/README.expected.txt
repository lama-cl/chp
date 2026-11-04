C5h0 expected behavior for p47:

- Target facts must use full nested field paths:
    run::ctx.ops.process   -> handle_int
    run::ctx.other.process -> handle_char

- Call resolution must also use the full field path:
    run via ctx.ops.process   -> handle_int
    run via ctx.other.process -> handle_char

- run.$0 / x should receive only handle_int:
    run($0 : void * => handle_int⌜int *⌝handle_int)

- run.$1 / y should receive only handle_char:
    run($1 : void * => handle_char⌜char *⌝handle_char)

- Negative checks:
    handle_char must not leak into x.
    handle_int must not leak into y.
    process fields under different parent paths must not be merged by field name.

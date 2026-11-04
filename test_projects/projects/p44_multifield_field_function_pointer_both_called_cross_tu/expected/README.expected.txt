C5h0 expected behavior for p44:

- Field-sensitive targets must be distinct:
    run::ops.parse -> parse_int
    run::ops.emit  -> emit_char

- Field-sensitive call resolution must contain two independent calls:
    run via ops.parse -> parse_int
    run via ops.emit  -> emit_char

- Because both fields are called on the same argument x, run.$0 should receive
  both target-origin histories, preferably as a parallel join:
    run($0 : void * => ⟨| parse_int⌜int *⌝parse_int,
                         emit_char⌜char *⌝emit_char |⟩)
  Arm order may vary.

- The important negative check is that target sets are not merged before calls;
  each call should resolve only through its own field path.

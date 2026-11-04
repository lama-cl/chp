C6d8 diagnostic target: nested data-field path through a same-TU
field-function-pointer call.

The field-function-pointer target consume_ctx has the param-relative field path:
  consume_ctx::c.box.payload

The call ops.consume(&ctx) should map that path to:
  run::ctx.box.payload

Expected core evidence:
  consume_ctx::c.box.payload : void * => int *
  run::ctx.box.payload      : void * => int *
  run::x                    : void * => int *
  run($0 : void * => int *)

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field, source=same-tu-data-field-field-fnptr or equivalent,
edge=data-field-field-function-pointer-call, and an int * deref-readwrite
operation through field=run::ctx.box.payload.

If p73 passes but this fails, ordinary function-pointer replay handles nested
paths but field-function-pointer replay is missing the nested path composition.

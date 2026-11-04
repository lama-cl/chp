C6d8 diagnostic target: nested data-field path through a cross-TU
field-function-pointer call.

The imported target consume_ctx has the param-relative field path:
  consume_ctx::c.box.payload

In import mode for a.c, the call ops.consume(&ctx) should map that path to:
  run::ctx.box.payload

Expected core evidence:
  consume_ctx::c.box.payload : void * => int *
  run::ctx.box.payload      : void * => int *
  run::x                    : void * => int *
  run($0 : void * => int *)

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field, source=summary-import or imported-field-function-pointer
provenance, edge=data-field-field-function-pointer-call, and an int *
deref-readwrite operation through field=run::ctx.box.payload.

If same-TU p74 passes but this fails, nested data-field paths are replayed
through local field-function-pointer targets but not through imported target
summaries.

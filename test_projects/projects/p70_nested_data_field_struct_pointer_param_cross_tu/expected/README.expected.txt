C6d7 diagnostic target: cross-TU nested data-field path through a struct-pointer
parameter.

The callee-relative field path in b.c is:
  consume_ctx::c.box.payload

In import mode for a.c, the call consume_ctx(&ctx) should map that path to:
  run::ctx.box.payload

Expected core evidence:
  consume_ctx::c.box.payload : void * => int *
  run::ctx.box.payload      : void * => int *
  run::x                    : void * => int *
  run($0 : void * => int *)

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field, source=summary-import, edge=data-field-param-call, and
an int * deref-readwrite operation through field=run::ctx.box.payload.

If same-TU p68 passes but this fails, then nested data-field summaries work
locally but are not fully exported/imported across TUs.

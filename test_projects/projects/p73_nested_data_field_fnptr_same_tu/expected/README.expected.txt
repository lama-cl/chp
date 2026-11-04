C6d8 diagnostic target: nested data-field path through an ordinary same-TU
function-pointer call.

The local target consume_ctx has the param-relative field path:
  consume_ctx::c.box.payload

The call cb(&ctx) should map that path to:
  run::ctx.box.payload

Expected core evidence:
  consume_ctx::c.box.payload : void * => int *
  run::ctx.box.payload      : void * => int *
  run::x                    : void * => int *
  run($0 : void * => int *)

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field and edge=data-field-function-pointer-call or equivalent
same-TU function-pointer data-field provenance, with an int * deref-readwrite
operation through field=run::ctx.box.payload.

If p68/p70 pass but this fails, nested data-field paths work for direct calls
but are not fully replayed through ordinary same-TU function-pointer calls.

C6d6 diagnostic target: nested data-field path through a same-TU struct-pointer
parameter.

The callee-relative field path is:
  consume_ctx::c.box.payload

At the call consume_ctx(&ctx), it should map to:
  run::ctx.box.payload

Expected if param-relative nested data-field summaries are supported:
  consume_ctx::c.box.payload : void * => int *
  run::ctx.box.payload      : void * => int *
  run::x                    : void * => int *
  run($0 : void * => int *)

Pointer-use summaries should propagate deref-readwrite as=int * from the callee
field use back to run::ctx.box.payload and then to the stored origin run::x.

If p67 passes but p68 fails, then local nested data-field extraction works but
same-TU parameter-field substitution does not yet support multi-segment paths.

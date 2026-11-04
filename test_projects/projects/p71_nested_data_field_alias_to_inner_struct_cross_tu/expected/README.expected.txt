C6d7 diagnostic target: cross-TU struct-pointer alias to an inner nested field
object.

This is the cross-TU analogue of p69:
  struct Box *bp = &ctx.box;
  consume_box(bp);

Expected core evidence in import mode for a.c:
  consume_box::b.payload : void * => int *
  run::ctx.box.payload  : void * => int *
  run::x                : void * => int *
  run($0 : void * => int *)

The important composition is:
  bp -> run::ctx.box
  callee b.payload -> run::ctx.box.payload
  run::ctx.box.payload stores run::x

The output should not materialize only run::ctx.payload or run::bp.payload as
the final semantic field location. Those may appear as raw evidence in some
intermediate views, but run::ctx.box.payload and run::x should receive the
imported field-derived cast and deref/write behavior.

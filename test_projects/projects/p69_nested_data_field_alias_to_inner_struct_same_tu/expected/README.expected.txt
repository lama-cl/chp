C6d6 diagnostic target: struct-pointer alias to an inner nested field object.

This is the nested analogue of p66.  p66 tested:
  struct Box *bp = &box;
  consume_box(bp);

This test checks the harder alias:
  struct Box *bp = &ctx.box;
  consume_box(bp);

Expected if struct-pointer aliases can target nested field objects:
  consume_box::b.payload : void * => int *
  run::ctx.box.payload  : void * => int *
  run::x                : void * => int *
  run($0 : void * => int *)

The important alias relation is:
  bp -> run::ctx.box

Then the callee-relative path b.payload should substitute to:
  run::ctx.box.payload

If p67 and p68 pass but p69 fails, then nested field extraction/substitution is
working, but struct-pointer aliases still need to map to a field path rather
than only to a base ValueDecl object.

C6d6b expected fix:
  The projected caller field must be run::ctx.box.payload, not run::ctx.payload
  and not run::bp.payload.  The field-path prefix carried by bp -> ctx.box
  must be prepended before the callee-relative payload path.

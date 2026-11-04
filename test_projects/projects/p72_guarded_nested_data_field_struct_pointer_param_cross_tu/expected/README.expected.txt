C6d7 diagnostic target: guarded cross-TU nested data-field path propagation.

The imported callees both use the same relative nested field path:
  consume_int_ctx::c.box.payload  -> int *
  consume_char_ctx::c.box.payload -> char *

In import mode for a.c, the caller-side histories should preserve the source
branch guard:
  run::ctx.box.payload : void * => <[tag] int *, [!(tag)] char *>
  run::x               : void * => <[tag] int *, [!(tag)] char *>
  run($0 : void * => <[tag] int *, [!(tag)] char *>, $1 : int)

The key diagnostic is that the result should be branch-sensitive, not an
unguarded parallel cast <| int *, char * |>.

Pointer-use summaries should likewise show guarded int * and char *
deref-readwrite rows with origin=data-field, source=summary-import, and
edge=data-field-param-call through field=run::ctx.box.payload.

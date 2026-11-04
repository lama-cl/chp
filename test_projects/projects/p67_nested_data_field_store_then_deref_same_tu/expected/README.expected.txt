C6d6 diagnostic target: nested local data-field store/load/deref.

This checks whether the existing C6d data-field machinery already treats
ctx.box.payload as a single field-storage identity, not merely as the final
field name payload.

Expected if nested data-field paths are supported:
  run::ctx.box.payload : void * => int *
  run::x               : void * => int *
  run($0 : void * => int *)

Pointer-use summaries / grouped rows should show a propagated deref-readwrite
as=int * on run::x through:
  field=run::ctx.box.payload
  origin=data-field

If this fails while simple box.payload cases pass, then C6d's data-field
extraction is still single-field-path oriented even though nested function-
pointer field paths are supported by earlier p47-style machinery.

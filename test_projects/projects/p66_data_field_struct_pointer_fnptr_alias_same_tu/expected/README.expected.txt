C6d5 regression target: same-TU ordinary function-pointer data-field projection through a struct-pointer alias.

For bp = &box; cb(bp), bp should map back to run::box before applying the
callee's relative field path b.payload.

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run($0 : void * => int *)

Pointer-use rows should show deref-readwrite as=int * on run::x through
run::box.payload with:
  source=same-tu-data-field-fnptr
  edge=data-field-function-pointer-call

C6d5b note:
  The same-TU ordinary function-pointer replay must snapshot the caller's
  struct-pointer object aliases at the call site.  Otherwise cb(bp) maps the
  callee relative field b.payload to run::bp.payload instead of the canonical
  run::box.payload, and the field-origin continuation to run::x is lost.

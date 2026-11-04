C6d2 regression target: cross-TU direct-call import of relative data-field summaries.

In import mode for a.c, consume_box(&box) should import the callee-side
consume_box::b.payload interpretation/use and materialize it on:
  - run::box.payload
  - run::x, through the prior box.payload = x store

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *
  run::x pointer-use rows include origin=data-field, source=summary-import,
  edge=data-field-param-call, and deref-readwrite as=int *.

C6d2 regression target: imported data-field summaries that were produced by a
same-TU callee inside the imported TU.

b.c summarizes consume_box::b.payload from consume_int(b->payload).  a.c import
mode should project that relative field behavior through consume_box(&box) to:
  - run::box.payload
  - run::x
with deref-readwrite as=int *.

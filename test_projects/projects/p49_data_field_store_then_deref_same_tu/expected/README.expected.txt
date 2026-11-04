Expected C6d behavior:
  box.payload is a data field, not a function-pointer field.

  The store:
    box.payload = x;
  should establish field-origin state from box.payload back to run::x.

  The later load/deref:
    int *p = box.payload;
    *p += 1;
  should report deref-readwrite as int * both on run::box.payload and on run::x.

  The run::x pointer-use rows should include structured data-field provenance:
    origin=data-field, field=run::box.payload, edge=data-field-alias/load

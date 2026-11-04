Expected C6d behavior:
  The guarded stores establish two field-origin alternatives:
    [tag]     box.payload stores a
    [!(tag)]  box.payload stores b

  The later dereference through a load from box.payload should propagate the
  deref-readwrite as int* operation back to both run::a and run::b with the
  corresponding guards and data-field provenance:
    [tag]     origin=data-field, field=run::box.payload, deref-readwrite as=int*
    [!(tag)]  origin=data-field, field=run::box.payload, deref-readwrite as=int*

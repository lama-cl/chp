C6d1b expected behavior:

- consume_box::b.payload records the local field interpretation and operation
  as int*.
- consume_box(&box) maps the relative field summary b.payload to the caller
  field run::box.payload.
- run::box.payload receives the transferred cast-history observation:

    run::box.payload : void * => int *

- run::x also receives the transferred cast-history observation through the
  stored data-field origin:

    run::x : void * => int *

- The corresponding deref-readwrite pointer operation is transferred to both
  run::box.payload and run::x with data-field-param-call provenance:

    origin=data-field, field=run::box.payload, source=same-tu-data-field-param-call,
    edge=data-field-param-call, as=int *, access=int : deref-readwrite

This verifies that interpretations and operations on fields of a struct-pointer
parameter are projected to the caller-side field/object reached by &box, not
misreported as operations on the struct pointer itself.

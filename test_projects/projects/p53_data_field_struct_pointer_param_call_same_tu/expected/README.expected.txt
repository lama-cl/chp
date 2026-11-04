C6d1b expected behavior:

- consume_box forwards b->payload to consume().
- consume's char* cast/pointer-use summary is materialized on
  consume_box::b.payload.
- consume_box(&box) maps b.payload to run::box.payload.
- run::box.payload and then run::x receive the transferred char* cast-history
  observation and the char* deref-readwrite operation with
  data-field-param-call provenance.

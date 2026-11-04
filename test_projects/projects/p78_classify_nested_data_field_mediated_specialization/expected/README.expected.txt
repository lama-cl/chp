C6e diagnostic target: nested data-field-mediated concrete specialization.

The parameter x is stored in the nested field path ctx.box.payload and later
used as int * through that same nested field.

Expected core evidence:
  run::ctx.box.payload : void * => int *
  run::x               : void * => int *

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field evidence through field=run::ctx.box.payload and a typed
int * deref-readwrite operation.

Ideal C6e semantic classification:
  entity: run::x
  behavior: NestedDataFieldMediatedConcreteSpecialization
  concrete target types: int *
  mediation path: run::ctx.box.payload
  callback-mediated: no
  data-field-mediated: yes
  nested field path: yes
  guarded alternatives: none
  unresolved/external dependency: no

This test checks whether classification/reporting preserves the nested field
path rather than collapsing it to payload alone.

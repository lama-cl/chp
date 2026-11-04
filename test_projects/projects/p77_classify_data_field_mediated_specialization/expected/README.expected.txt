C6e diagnostic target: data-field-mediated concrete specialization.

The parameter x is not cast directly. It is stored in box.payload and later
loaded from that field as int *.

Expected core evidence:
  run::box.payload : void * => int *
  run::x           : void * => int *

Pointer-use summaries / grouped rows for run::x should include propagated
origin=data-field evidence through field=run::box.payload and a typed
int * deref-readwrite operation.

Ideal C6e semantic classification:
  entity: run::x
  behavior: DataFieldMediatedConcreteSpecialization
  concrete target types: int *
  mediation path: run::box.payload
  callback-mediated: no
  data-field-mediated: yes
  nested field path: no
  guarded alternatives: none
  unresolved/external dependency: no

This should be distinguished from DirectConcreteSpecialization because the
interpretation is mediated by a storage field.

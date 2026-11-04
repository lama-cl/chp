C6e diagnostic target: direct concrete specialization classification.

This is the simplest structured void * use. The parameter x is directly
interpreted as int * and then dereferenced/written.

Expected core evidence:
  run::x : void * => int *

Pointer-use summaries / grouped rows for run::x or the propagated parameter
behavior should include a typed deref-readwrite as int * with local/direct
provenance.

Ideal C6e semantic classification:
  entity: run::x
  behavior: DirectConcreteSpecialization
  concrete target types: int *
  mediation: none
  callback-mediated: no
  data-field-mediated: no
  guarded alternatives: none
  unresolved/external dependency: no

This test is intended to check that a direct, resolved concrete use is not
classified as WildPointer or ExternalUnknown.

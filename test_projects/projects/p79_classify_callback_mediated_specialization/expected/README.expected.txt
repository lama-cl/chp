C6e diagnostic target: callback-mediated concrete specialization.

The parameter x is passed through a known local function pointer target. The
concrete interpretation occurs in handle_int, not syntactically inside run.

Expected core evidence:
  handle_int::x : void * => int *
  run::x        : void * => int *

The output may show the propagated history using a callback wrapper such as
handle_int⌜int *⌝handle_int or an equivalent function-pointer/import provenance.
Pointer-use summaries for run::x should include call-argument/callback-mediated
propagation to a typed int * deref-readwrite operation.

Ideal C6e semantic classification:
  entity: run::x
  behavior: CallbackMediatedConcreteSpecialization
  concrete target types: int *
  callback targets: handle_int
  mediation: function pointer cb
  data-field-mediated: no
  guarded alternatives: none
  unresolved/external dependency: no

This should be distinguished from WildPointer because the callback target is
resolved and has a concrete summary.

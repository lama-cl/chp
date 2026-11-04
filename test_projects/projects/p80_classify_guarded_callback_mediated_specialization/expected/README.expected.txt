C6e diagnostic target: guarded callback-mediated specialization.

The parameter x is passed through a function pointer whose target is selected
by tag. The desired behavior is a guarded set of concrete alternatives, not an
unstructured parallel/wild summary.

Expected core evidence:
  handle_int::x  : void * => int *
  handle_char::x : void * => char *
  run::x         : void * => ⟨[tag] int *, [!(tag)] char *⟩

Equivalent wrapped callback histories are acceptable as long as the branch
conditions are preserved and the two alternatives are not collapsed into
unguarded parallel casts.

Pointer-use summaries / grouped rows for run::x should include guarded typed
operations:
  [tag]    deref-readwrite as=int *
  [!(tag)] deref-readwrite as=char *

Ideal C6e semantic classification:
  entity: run::x
  behavior: GuardedCallbackMediatedSpecialization
  concrete target types: int *, char *
  callback targets: handle_int, handle_char
  mediation: function pointer cb
  data-field-mediated: no
  guarded alternatives:
    [tag]    int * via handle_int
    [!(tag)] char * via handle_char
  unresolved/external dependency: no

This is one of the key cases for arguing that some apparent polymorphic void *
uses are structured by branch/tag logic.

C6e diagnostic target: unresolved external boundary classification.

The parameter x is passed to an external function with no available definition
or imported summary. Unlike p76-p80, this case really should remain unknown or
external-boundary dependent.

Expected core evidence:
  run::x has call-arg/call-argument-flow evidence to sink

There may be no concrete cast history such as int * or char * because the target
summary is unavailable.

Ideal C6e semantic classification:
  entity: run::x
  behavior: ExternalUnknownBoundary or ExternalSink
  concrete target types: none known
  mediation: direct call to unresolved sink
  data-field-mediated: no
  callback-mediated: no
  guarded alternatives: none
  unresolved/external dependency: yes

This test is intended to keep the ExternalUnknown label reserved for true
unresolved boundaries, not resolved functions whose names merely contain words
such as external.

C6f1 classification-lock target: ExternalUnknownBoundary.

This locks the true external-boundary case. The target sink has no body and no
available summary, so the tool should preserve the unresolved-boundary signal.

Expected core markers:
  Pointer use summaries / Semantic pointer-operation summaries:
    run::x has call-arg / call-argument-flow evidence to sink

  Pointer behavior metrics / Function parameter behavior metrics:
    semanticMediation: kind=ExternalBoundary
    semanticFamily=ExternalUnknownBoundary
    targets=sink
    edges=external-direct-call
    flags: externalBoundary=yes

Negative lock markers:
  This should not be downgraded to Direct, Callback, DataField, or
  ConcreteSpecialization merely because sink is syntactically a direct call.

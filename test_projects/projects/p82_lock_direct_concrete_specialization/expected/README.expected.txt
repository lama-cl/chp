C6f1 classification-lock target: DirectConcreteSpecialization.

This test locks the baseline direct case. The parameter x is interpreted as
int * directly inside run, without data-field, callback, return-alias, or
external-boundary mediation.

Expected core markers:
  Direct cast histories:
    run::x : void * => int *

  Function summaries:
    run($0 : void * => int *)

  Pointer behavior metrics / Function parameter behavior metrics:
    behavior: ConcreteSpecializedWrite
    paperWildness: surfaceKind=void-interface, semanticFamily=ConcreteSpecialization
    semanticMediation: kind=Direct, semanticFamily=ConcreteSpecialization

Negative lock markers:
  semanticMediation should not report kind=DataField, kind=NestedDataField,
  kind=Callback, or kind=ExternalBoundary for run::x / run.$0.
  It should not report edges=external-direct-call.

C6f1 classification-lock target: NestedDataFieldMediatedConcreteSpecialization.

This locks nested field-path mediation after C6d6b/C6e1.

Expected core markers:
  Direct cast histories:
    run::x : void * => int *
    run::ctx.box.payload : void * => int *

  Function summaries:
    run($0 : void * => int *)

  semanticMediation for run::x / run.$0:
    kind=NestedDataField
    semanticFamily=NestedDataFieldMediatedConcreteSpecialization
    fields=run::ctx.box.payload
    flags: dataField=yes, nestedDataField=yes, callback=no, externalBoundary=no

Negative lock markers:
  This should not be reduced to fields=run::ctx.payload or run::bp.payload.
  It should not report kind=ExternalBoundary or edges=external-direct-call.

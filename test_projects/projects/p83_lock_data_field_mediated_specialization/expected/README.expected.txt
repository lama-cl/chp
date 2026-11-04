C6f1 classification-lock target: DataFieldMediatedConcreteSpecialization.

This is the p77-style regression lock. The interpretation of x reaches a
concrete int * dereference through box.payload.

Expected core markers:
  Direct cast histories:
    run::x : void * => int *
    run::box.payload : void * => int *

  Function summaries:
    run($0 : void * => int *)

  semanticMediation for run::x / run.$0:
    kind=DataField
    semanticFamily=DataFieldMediatedConcreteSpecialization
    fields=run::box.payload
    edges should include data-field-load and/or data-field-alias
    flags: dataField=yes, nestedDataField=no, callback=no, externalBoundary=no

Negative lock markers:
  This should not collapse back to kind=Direct.
  It should not report kind=ExternalBoundary or edges=external-direct-call.

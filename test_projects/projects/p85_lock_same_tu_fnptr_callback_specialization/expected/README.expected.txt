C6f1 classification-lock target: CallbackMediatedConcreteSpecialization.

This locks the p79-style same-TU ordinary function-pointer case. The concrete
interpretation occurs in handle_int and is projected to run::x through cb(x).

Expected core markers:
  Direct cast histories:
    handle_int::x : void * => int *
    run::x : void * => handle_int⌜int *⌝handle_int

  Function summaries:
    handle_int($0 : void * => int *)
    run($0 : void * => handle_int⌜int *⌝handle_int)

  Pointer behavior metrics / Function parameter behavior metrics for run::x/run.$0:
    behavior: ConcreteSpecializedWrite
    semanticMediation: kind=Callback
    semanticFamily=CallbackMediatedConcreteSpecialization
    targets=handle_int
    edges should include function-pointer-call or higher-order-callback
    flags: callback=yes, externalBoundary=no

Negative lock markers:
  run::x should not be missing from Direct cast histories.
  run($0 : void *) should not remain un-specialized.
  The mediation should not be kind=Direct or kind=ExternalBoundary.

C6f1 classification-lock target: callback plus return-alias evidence remains stable.

This locks an aggregate case similar to the demo examples: local concrete/byte
interpretations, a higher-order call, guarded function-pointer alternatives,
and return-alias continuations can all contribute to the same caller-side entity.

This test intentionally avoids the conditional operator:

  void *(*pf)(void *) = tag ? idv : fret;

because ternary function-pointer target-set recovery is a separate language
feature. The C6f1 lock should test classification/reporting stability, not add
new conditional-expression target resolution.

Expected core markers:
  Function summaries should report ret aliases for idv and fret:
    idv(... ) -> void * [...] [ret aliases $0]
    fret(... ) -> void * [...] [ret aliases $0]

  ho should remain higher-order/callback dependent:
    behavior: CallbackDependent...
    semanticMediation: kind=Callback
    edges should include higher-order-call or higher-order-callback
    externalBoundary=no

  demo::pi or demo::pv should carry callback/higher-order evidence through ho.
  Return-alias contribution should remain visible when present:
    returnAlias=yes

  If guarded alternatives are preserved, targets should include both:
    idv
    fret

  Guarded function-pointer alternatives should be reflected in caller-side
  histories/metrics when pf is passed through ho and then invoked as pf(...):
    [tag] idv / int *
    [!(tag)] fret / char *
  In particular, demo::pv and/or demo::pi should not remain only as the
  unresolved symbolic placeholder ho⌜char * => void * => $0.$0⌝ho.

Negative lock markers:
  The aggregate caller-side mediation should not contain edges=external-direct-call
  when externalBoundary=no.
  Return-alias evidence should not force ExternalUnknownBoundary.
  This project should not require conditional-expression target recovery.

C6f1 classification-lock target: GuardedCallbackMediatedSpecialization.

This locks the p80-style guarded ordinary function-pointer target set.

Expected core markers:
  Direct cast histories should include handle_int::x and handle_char::x.
  run::x should receive guarded alternatives, not an unguarded parallel-only
  collapse:
    [tag] handle_int⌜int *⌝handle_int
    [!(tag)] handle_char⌜char *⌝handle_char

  Function summaries:
    run($0 : void * => ... guarded callback alternatives ..., $1 : int)

  semanticMediation for run::x / run.$0:
    kind=Callback
    semanticFamily=GuardedCallbackMediatedSpecialization
    targets should include handle_int and handle_char
    flags: callback=yes, externalBoundary=no

Negative lock markers:
  The mediation should not be Direct or ExternalBoundary.
  The branch structure should not lose [tag] / [!(tag)] guards.

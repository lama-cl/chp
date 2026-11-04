C6g4z2 concrete identity callback regression

`invoke_predicate` has generic higher-order summaries:

  invoke_predicate.$1 : const void * => $0.$0
  invoke_predicate.$2 : const void * => $0.$1

At `run_identity_predicate`, `$0` is bound to `identity_predicate`, whose two
parameters have no cast observations.  Those selected callback formals are
therefore identity in the cast-history domain.

Required result:
  - `run_identity_predicate.$0` must not contain `$0.$0`, `$0.$1`, or an
    `<unknown:identity_predicate.$N-unsummarized>` leaf.
  - No synthetic `const void *` identity arm is required in the function
    summary.  The absence of a cast summary is the canonical representation of
    a concrete cast-free callback use.

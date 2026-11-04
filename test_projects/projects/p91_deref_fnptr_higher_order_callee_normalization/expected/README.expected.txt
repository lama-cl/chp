C6g3 + C6g4b/C6g4c regression target: dereferenced function-pointer callee
normalization plus resolved named-direct-call specialization of callback
summaries.

C treats these call spellings as equivalent:
  compare(key, candidate);
  (*compare)(key, candidate);

C6g3 requirement:
  - Both invoke_direct and invoke_deref recognize compare as function parameter
    $0 and retain matching symbolic effects $0.$0 and $0.$1 in their own
    generic summaries.
  - invoke_deref must not create a separate *compare callback identity.

C6g4b visitor requirement:
  - At exercise(compare_ints, key, base, ...), the deferred named-call solver
    binds invoke_{direct,deref}.$0 to the uniquely resolved actual callback
    compare_ints.
  - The solver substitutes the callback target into the callee semantic core;
    it does not retain an unresolved $0.$0/$0.$1 sibling and does not add a
    generic/concrete Parallel merely because the specialization was deferred.
  - Report wrappers are excluded from the solver core without losing their
    inner semantics.  In particular, the base path retains
      const char * => ptrarith(const char *) => const void *
    from at_index before reaching compare_ints.$1.
  - exercise receives two provenance-distinct routes (invoke_direct and
    invoke_deref).  Both normalize to the same concrete semantic histories;
    the resulting duplicate Parallel arms simplify to one history.

Expected solved histories:
  exercise.$0 = const int * => const void * => const int *
  exercise.$1 = const int * => const void * => const char *
                => ptrarith(const char *) => const void * => const int *

The direct entity histories for exercise::key and exercise::base should match
these solved histories.  The generic invoke_direct and invoke_deref summaries
remain polymorphic and may still contain $0.$0 and $0.$1.

C6g4c project-index requirement:
  - summaryParamTransferEdges for the two exercise -> invoke_* call sites
    export the uniquely resolved binding invoke_*.$0 -> compare_ints.
  - build_summary_index.py substitutes that binding into the callee semantic
    core before composing the caller data-argument path.
  - C2 must not reintroduce a symbolic $0.$0/$0.$1 arm after C6g4b has
    already specialized the C1 local summary.

Run both final-output verifiers and the index verifier after the complete
multitu workflow.

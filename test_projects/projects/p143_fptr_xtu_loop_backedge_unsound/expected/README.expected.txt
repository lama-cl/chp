Expected fptr-extension behavior for p143_fptr_xtu_loop_backedge_unsound.

Fact: X12 xtu-loop-backedge-unsound

Fact X12 (cross-TU F10, SOUNDNESS BUG today): the callback is reassigned on
the loop back edge; both targets are defined in b.c.  Only use_int's
imported summary is composed today.

Subject run::ctx:
  today (pinned baseline):  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
  required after extension: category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p143_fptr_xtu_loop_backedge_unsound
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

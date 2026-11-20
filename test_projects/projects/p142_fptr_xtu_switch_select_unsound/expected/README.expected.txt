Expected fptr-extension behavior for p142_fptr_xtu_switch_select_unsound.

Fact: X11 xtu-switch-select-unsound

Fact X11 (cross-TU F09, SOUNDNESS BUG today): switch arms select between two
targets defined in b.c.  The syntactic switch traversal keeps only the last
assignment, so the imported summary of use_int is never composed.

Subject run::ctx:
  today (pinned baseline):  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double*
  required after extension: category=GuardedActiveReinterpretation types=double*,int* resolution=set@L0

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p142_fptr_xtu_switch_select_unsound
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

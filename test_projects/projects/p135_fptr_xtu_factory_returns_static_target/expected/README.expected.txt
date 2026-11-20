Expected fptr-extension behavior for p135_fptr_xtu_factory_returns_static_target.

Fact: X04 xtu-factory-returns-static-target

Fact X04 (cross-TU F11): pick() in b.c returns a *static* function of b.c.
a.c cannot name use_int, so the returned target must be identified by a
TU-qualified lookup key (static functions are not public symbols).

Subject run::ctx:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p135_fptr_xtu_factory_returns_static_target
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

Expected fptr-extension behavior for p138_fptr_xtu_fsm_states.

Fact: X07 xtu-fsm-states

Fact X07 (cross-TU F17): the state functions live in b.c, the driver loop in
a.c.  The driver's target set is the fixpoint over returned next-state
targets, which are only visible in b.c's return summaries.

Subject run::ctx:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=set@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p138_fptr_xtu_fsm_states
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

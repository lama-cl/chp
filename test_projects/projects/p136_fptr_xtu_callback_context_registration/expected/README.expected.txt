Expected fptr-extension behavior for p136_fptr_xtu_callback_context_registration.

Fact: X05 xtu-callback-context-registration

Fact X05 (cross-TU F18): the registry (add/fire) is a library TU (b.c); the
client in a.c registers (use_int, x).  b.c's summary of add() must export a
callback-context binding "$0 is invoked later with $1 as its argument #0",
which a.c applies at the registration site.
Real world: c-thread-pool thpool_add_work (main.c:345) / worker call
(main.c:530).

Subject setup::x:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p136_fptr_xtu_callback_context_registration
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

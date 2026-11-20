Expected fptr-extension behavior for p133_fptr_xtu_global_registry_via_setter.

Fact: X02 xtu-global-registry-via-setter

Fact X02 (cross-TU F04): b.c owns a global hook and a setter API; a.c
registers its own handler through the setter.  The store into g_handler is
relative to the setter's parameter ($0), so the slot's target set is only
known after the caller binding set_handler(use_int) is applied project-wide.
The subject that benefits is in b.c (fire::ctx).
Real world: parson json_set_allocation_functions() (parson.c:2052).

Subject fire::ctx:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p133_fptr_xtu_global_registry_via_setter
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

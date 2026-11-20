Expected fptr-extension behavior for p140_fptr_xtu_library_callback_pthread.

Fact: X09 xtu-library-callback-pthread

Fact X09 (cross-TU F21): the thread entry point is defined in b.c and passed
to pthread_create in a.c.  The library model provides the binding
(fn=#2, data=#3 -> param #0); the target summary is imported from b.c.

Subject spawn::ctx:
  today (pinned baseline):  category=ExternalUnknown callback=None behavior=OpaqueForwarding types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@LIB

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p140_fptr_xtu_library_callback_pthread
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

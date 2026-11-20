Expected fptr-extension behavior for p137_fptr_xtu_callback_pair_correlation.

Fact: X06 xtu-callback-pair-correlation

Fact X06 (cross-TU F19, precision): two clients register different
(callback, context) pairs with the b.c registry.  Each context must be
classified by its own callback only; crossing the pairs would make both look
both int* and double*.

Subject setup_int::x:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L1

Subject setup_dbl::y:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
  required after extension: category=MonomorphicActiveSpecialization types=double* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p137_fptr_xtu_callback_pair_correlation
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

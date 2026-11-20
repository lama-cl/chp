Expected fptr-extension behavior for p144_fptr_xtu_job_struct_executor.

Fact: X13 xtu-job-struct-executor

Fact X13 (cross-TU F20): the executor exec(struct job *) is in b.c and calls
j->fn(j->arg); a.c builds the job with its own callback.  b.c's summary is
relative to $0->fn and $0->arg and must be specialized at a.c's call.
Real world: bzip2 bz_stream bzalloc/opaque (main.c:112, 7857).

Subject submit::j.arg:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L0

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p144_fptr_xtu_job_struct_executor
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

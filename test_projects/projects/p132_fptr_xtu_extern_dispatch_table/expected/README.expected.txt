Expected fptr-extension behavior for p132_fptr_xtu_extern_dispatch_table.

Fact: X01 xtu-extern-dispatch-table

Fact X01 (cross-TU F05): the dispatch table is defined and initialized in
b.c and indexed in a.c through an extern declaration.  The table's element
targets are only visible in b.c's TU, so a.c needs project-level L1 storage
facts for `table[*]` merged by build_summary_index.py.

Subject run::ctx:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=set@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p132_fptr_xtu_extern_dispatch_table
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

/*
 * Fact X01 (cross-TU F05): the dispatch table is defined and initialized in
 * b.c and indexed in a.c through an extern declaration.  The table's element
 * targets are only visible in b.c's TU, so a.c needs project-level L1 storage
 * facts for `table[*]` merged by build_summary_index.py.
 *
 * @fptr-fact X01 xtu-extern-dispatch-table
 * @subject   run::ctx
 * @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
 */
typedef int (*handler_t)(void *);

extern const handler_t table[2];

int run(unsigned op, void *ctx) { return table[op & 1](ctx); }

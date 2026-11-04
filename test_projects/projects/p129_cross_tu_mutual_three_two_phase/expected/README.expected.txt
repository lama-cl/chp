C6g5s regression target: p129 is the ordinary int* -> char* -> double*
recursive cast cycle split across f.c, g.c, and h.c.  It must not be skipped
as local-only during C2.  The project/import two-phase SCC solver must:

  - recognize at least one imported concrete edge in the shared SCC;
  - run project cast-history propagation to a finite semantic-fact fixpoint;
  - run pointer-use propagation only after project cast quiescence;
  - retain int*, char*, and double* cast evidence; and
  - never re-open the project cast phase from pointer-use changes.

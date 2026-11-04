This project exercises the first SCC-aware cross-TU parameter-summary fixpoint.

Topology:
  a.c: f -> relay -> g
  b.c: g -> f

The project-level dependency graph should contain one recursive concrete-function
SCC containing f and g. In import mode, the tool should report:

  Cross-TU summary SCC awareness:
    f : SCC #... [recursive]

and a non-empty:

  Cross-TU SCC parameter-summary fixpoint:
    recursive import edges: ...
    domains: parameter-cast-histories

The exact cast summary detail may depend on whether the project-summary-index was
built from local summaries only or from a later import-enabled pass. The main
regression goal for C4D is that SCC-aware recursive imported parameter
propagation is attempted and reported.

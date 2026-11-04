C4b expected behavior: cross-TU SCC reporting
================================================

This project contains a direct cross-TU recursive cycle:

  a.c:f -> b.c:g
  b.c:g -> a.c:f

After running the normal multi-TU workflow:

  1. run cast-history-tool over a.c and b.c to produce *.cast-summary.json
  2. run scripts/build_summary_index.py over those summaries

project-summary-index.json should contain summaryDependencySCCs with one
nontrivial concrete-function SCC containing both functions:

  summaryDependencySCCs.concreteFunctionGraph.numNontrivialSCCs == 1
  members include:
    f::fn:(ptr:void)->void
    g::fn:(ptr:void)->void

The full-summary graph should report the same cycle for this test, because
there are no symbolic callback nodes.

C4b intentionally does not solve recursive summaries yet.  It only builds the
project-level dependency graph and reports SCC topology.

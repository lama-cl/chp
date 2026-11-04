C4H expected behavior: project-level solved parameter summaries are propagated
from solved callees to downstream callers through exported param-transfer edges.

Shape:
  f -> relay -> g -> f forms the recursive SCC.
  h -> f depends on the SCC.
  k -> h depends on h.

The index builder should export summaryParamTransferEdges from the raw TU pass
and materialize solvedParamSummaries not only for SCC members, but also for
acyclic downstream callers such as h and k. The final cast-history-tool pass
should still be responsible for rendering final text output and classifications.

Important limitation in C4H: intra-SCC transfer propagation is deliberately not
solved in the project index yet; same-SCC transfer edges are skipped to avoid
reintroducing over-unrolled cycle histories. The next step is a bounded/widened
intra-SCC summary solver.

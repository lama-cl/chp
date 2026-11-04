C6g5t cross-TU parallel-fork materialization regression target.

p130 splits p126's relay_a int*/char* fork and relay_b short* relay across two
translation units.  C2 must run the project two-phase SCC solver because the
component has imported edges.

Required properties:
  - canonical project-solved parameter histories expose the two root-correlated
    arms int*=>void*=>short*=>void* and char*=>void*=>short*=>void*;
  - direct AST-local entity histories remain separately reported and are not
    silently overwritten by the derived project view;
  - project pointer-use begins only after project cast quiescence; and
  - C2 reports timing/rebuild metadata for each actual index build.

C6g5w1c metadata-only prerequisite:
  - The imported p130_relay_a parallel-call family must retain a
    `recursiveClosureKey` equal to its exact source family ID after C2 index
    transport and SummaryIndex rehydration.

C6g5w1d authoritative C2 behavior:
  - the p130_relay_a family consumed after cross-TU index rehydration must
    materialize normal solvedParamSummaries for both relays, matching p126.
  - b.c must report the consumed closure key and p130_relay_b.$0 must retain
    short * => void * => {int *, char *} => void * alternatives.

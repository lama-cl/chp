C6g5t local-only parallel-fork materialization regression target.

The two direct relay_a call arguments are a single source fork with distinct
arms:
  int *  => void * => short * => void *
  char * => void * => short * => void *

Required properties:
  - C1 cast and pointer-use SCC phases quiesce independently;
  - C2 records this component as local-only and does not run import/index
    refinement rounds;
  - one final C2 reporting overlay may expose canonical project summaries;
  - the canonical view retains both fork arms and their arm provenance;
  - no rec{...} marker is introduced.

C6g5w1c metadata-only prerequisite:
  - The p126 parallel-call family must expose `recursiveClosureKey` equal to
    its exact source family ID.

C6g5w1d authoritative C2 behavior:
  - when the parallel-call family carries a consumed recursiveClosureKey,
    ordinary solvedParamSummaries (and final imported Function summaries)
    retain exactly two arms:
      int * => void * => short * => void *
      char * => void * => short * => void *
  - the raw C1 local solver remains unchanged; verify the C2 imported run.

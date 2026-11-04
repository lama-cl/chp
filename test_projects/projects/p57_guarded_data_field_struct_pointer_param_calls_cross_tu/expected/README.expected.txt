C6d2 regression target: guarded cross-TU data-field direct calls.

In a.c import mode, branch guards should be preserved when projecting imported
relative data-field summaries:
  [tag]     run::box.payload / run::x use as int *
  [!(tag)]  run::box.payload / run::x use as char *

The two alternatives should remain branch-separated rather than becoming an
unguarded parallel multi-type use.

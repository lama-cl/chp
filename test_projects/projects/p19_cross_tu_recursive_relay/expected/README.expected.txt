This project exercises the motivation for C4D-full.

One import-mode pass can leave the cross-TU recursive summaries one round stale,
because a.c imports b.c from a fixed project-summary-index.json and b.c imports a.c
from the same fixed index.

Running scripts/run_scc_param_fixpoint.py over the two translation units should
iterate until the recursive concrete SCC stops changing.  At convergence, relay and
f should both reflect the int* / char* cycle instead of only one-round imported
prefixes.

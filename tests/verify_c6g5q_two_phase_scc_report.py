#!/usr/bin/env python3
"""Check C6g5q's phase-separation invariants in text report output."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("report")
parser.add_argument("--fixture", choices=("p124", "p126", "p127"), required=True)
args = parser.parse_args()
text = Path(args.report).read_text(errors="replace")

required = (
    "scheduler: scc-local-parameter-dependency-worklist",
    "phases: cast-history-then-pointer-use",
    "cast-history state: semantic atom/transition facts; rich histories retained for reporting",
    "phase dependency: pointer-use changes never reopen cast-history SCC solving",
    "pointer-use reopened cast SCC: 0",
    "termination: scc-worklist-quiescent",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"missing required C6g5q report field: {needle}")

for label in (
    "cast semantic fact candidates:",
    "cast semantic fact insertions:",
    "cast semantic fact subsumed:",
    "pointer-use phase:",
    "pointer-use edge executions:",
):
    if label not in text:
        raise SystemExit(f"missing C6g5q phase counter: {label}")

if "rec{" in text:
    raise SystemExit("unexpected recursive-condition marker in C6g5q regression output")

if args.fixture == "p124":
    for evidence in ("int *", "char *", "double *", "h : REINTERPRETING_OBSERVED"):
        if evidence not in text:
            raise SystemExit(f"p124 missing required preserved evidence: {evidence}")
elif args.fixture == "p126":
    for evidence in ("int *", "char *", "short *"):
        if evidence not in text:
            raise SystemExit(f"p126 missing cast evidence: {evidence}")
elif args.fixture == "p127":
    for evidence in ("unsigned char *", "tag", "!(tag)"):
        if evidence not in text:
            raise SystemExit(f"p127 missing guarded byte-pointer evidence: {evidence}")

print(f"C6g5q {args.fixture} two-phase report: OK")

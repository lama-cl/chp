#!/usr/bin/env python3
"""Check C6g5r's phase-2 operation-set invariants in text report output."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("report")
parser.add_argument("--fixture", choices=("p124", "p126", "p127", "p128"), required=True)
args = parser.parse_args()
text = Path(args.report).read_text(errors="replace")

required = (
    "scheduler: scc-local-parameter-dependency-worklist",
    "phases: cast-history-then-pointer-use",
    "phase dependency: pointer-use changes never reopen cast-history SCC solving",
    "pointer-use reopened cast SCC: 0",
    "pointer-use termination: pointer-use-scc-worklist-quiescent",
    "termination: scc-worklist-quiescent",
    "pointer-use operation candidates:",
    "pointer-use operation insertions:",
    "pointer-use operation subsumed:",
    "pointer-use frozen cast-attachment overlays:",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"missing required C6g5r report field: {needle}")

if "rec{" in text:
    raise SystemExit("unexpected recursive-condition marker in C6g5r regression output")
if "execution-limit" in text:
    raise SystemExit("C6g5r pointer-use SCC did not quiesce")

match = re.search(r"pointer-use edge executions: (\d+) / (\d+)", text)
if not match:
    raise SystemExit("missing pointer-use execution budget")
if int(match.group(1)) >= int(match.group(2)):
    raise SystemExit("pointer-use phase consumed its full edge budget")

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
elif args.fixture == "p128":
    for evidence in ("int *", "char *", "deref-readwrite"):
        if evidence not in text:
            raise SystemExit(f"p128 missing pointer-use evidence: {evidence}")

print(f"C6g5r {args.fixture} two-phase operation-set report: OK")

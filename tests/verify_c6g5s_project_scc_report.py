#!/usr/bin/env python3
"""Validate C6g5s project-level two-phase SCC report fields."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("report")
parser.add_argument("--fixture", choices=("p126", "p129", "p130"), required=True)
args = parser.parse_args()
text = Path(args.report).read_text(errors="replace")

required = (
    "Cross-TU SCC parameter-summary fixpoint:",
    "scheduler: project-scc-two-phase",
    "phases: cast-history-then-pointer-use",
    "phase dependency: pointer-use changes never reopen project cast-history SCC solving",
    "pointer-use reopened project cast SCC: 0",
    "domains: parameter-cast-histories, parameter-pointer-uses",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"missing required C6g5s report field: {needle}")

if "project-pointer-use-scc-iteration-limit" in text or "cast-history-iteration-limit" in text:
    raise SystemExit("C6g5s project SCC did not quiesce")

if args.fixture == "p126":
    if "note: skipped-local-only-sccs" not in text:
        raise SystemExit("p126 must skip replay of its C1-complete local-only SCC")
elif args.fixture == "p129":
    for evidence in ("int *", "char *", "double *"):
        if evidence not in text:
            raise SystemExit(f"p129 missing cross-TU cast evidence: {evidence}")
    match = re.search(r"recursive project SCC edges: (\d+)", text)
    if not match or int(match.group(1)) == 0:
        raise SystemExit("p129 did not enter the project SCC solver")
elif args.fixture == "p130":
    for evidence in ("int *", "char *", "short *"):
        if evidence not in text:
            raise SystemExit(f"p130 missing cross-TU parallel cast evidence: {evidence}")

print(f"C6g5s {args.fixture} project two-phase report: OK")

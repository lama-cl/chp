#!/usr/bin/env python3
"""Validate C6g5t canonical project-history report sections."""
from __future__ import annotations

import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("report")
parser.add_argument("--fixture", choices=("p126", "p127", "p130"), required=True)
args = parser.parse_args()
text = Path(args.report).read_text(errors="replace")

required = (
    "Project-solved parameter histories (canonical):",
    "[derived-project-summary]",
    "canonical C2 materialization; direct AST-local entity histories above are not rewritten by this view.",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"missing C6g5t canonical-report field: {needle}")

if args.fixture == "p126":
    for needle in (
        "relay_a.$0",
        "int * => void * => short * => void *",
        "char * => void * => short * => void *",
        "call-actual-fork",
    ):
        if needle not in text:
            raise SystemExit(f"p126 missing canonical fork/arm evidence: {needle}")
elif args.fixture == "p127":
    for needle in (
        "f.$0",
        "ptrarith(unsigned char *)",
        "[tag]",
        "[!(tag)]",
    ):
        if needle not in text:
            raise SystemExit(f"p127 missing canonical guarded-transformer evidence: {needle}")
    section = text.split("Project-solved parameter histories (canonical):", 1)[1]
    # The canonical derived entry should be bounded: two guarded PtrArith arms,
    # not a recursive tower.  Other report sections may preserve raw evidence.
    entry = section.split("Recursive named-call fixpoint:", 1)[0]
    if entry.count("ptrarith(unsigned char *)") > 8:
        raise SystemExit("p127 canonical section still contains a recursive PtrArith tower")
elif args.fixture == "p130":
    for needle in (
        "p130_relay_a.$0",
        "int * => void * => short * => void *",
        "char * => void * => short * => void *",
        "fork=",
    ):
        if needle not in text:
            raise SystemExit(f"p130 missing project-solved fork/arm evidence: {needle}")

print(f"C6g5t {args.fixture} canonical project report: OK")

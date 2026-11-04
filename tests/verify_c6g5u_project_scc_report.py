#!/usr/bin/env python3
"""Validate the C6g5u canonical C2 materialization regressions."""
from __future__ import annotations

import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("report")
parser.add_argument("--fixture", choices=("p126", "p127", "p23", "p129", "p130"), required=True)
args = parser.parse_args()
text = Path(args.report).read_text(errors="replace")

if args.fixture == "p126":
    for needle in (
        "relay_a($0 : void * =>",
        "int * => void * => short * => void *",
        "char * => void * => short * => void *",
        "run_recursive_parallel_observations($0 : void * =>",
    ):
        if needle not in text:
            raise SystemExit(f"p126 missing C6g5u canonical history evidence: {needle}")
elif args.fixture == "p127":
    required = (
        "f($0 : unsigned char * => ⟨[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)⟩",
        "g($0 : unsigned char * => ⟨[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)⟩",
        "run_guarded_recursive_ptrarith_membership_projection($0 : unsigned char * => ⟨[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)⟩",
    )
    for needle in required:
        if needle not in text:
            raise SystemExit(f"p127 missing canonical forwarder summary: {needle}")
elif args.fixture == "p23":
    for needle in (
        "relay($0 : void * => int * => void * => char * => void *)",
        "f($0 : void * => char * => void * => int * => void *)",
    ):
        if needle not in text:
            raise SystemExit(f"p23 missing C6g5u cross-TU cycle continuation: {needle}")
elif args.fixture == "p129":
    needle = "p129_f($0 : void * => int * => void * => char * => void * => double * => void *"
    if needle not in text:
        raise SystemExit("p129 missing full int/char/double C6g5u continuation")
elif args.fixture == "p130":
    for needle in (
        "p130_relay_a.$0",
        "int * => void * => short * => void *",
        "char * => void * => short * => void *",
    ):
        if needle not in text:
            raise SystemExit(f"p130 missing C6g5u fork-aware continuation: {needle}")

print(f"C6g5u {args.fixture} project materialization: OK")

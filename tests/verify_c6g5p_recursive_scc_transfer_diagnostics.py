#!/usr/bin/env python3
"""Verify that a C6g5p recursive-transfer diagnostic log is structurally useful.

Usage:
  python3 tests/verify_c6g5p_recursive_scc_transfer_diagnostics.py LOG [--fixture p127]

This verifier is intentionally log-only: it does not run the analyzer and does
not interpret a bounded diagnostic fingerprint as a solver result.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log", type=Path)
    ap.add_argument("--fixture", choices=["p126", "p127"], required=True)
    args = ap.parse_args()
    text = args.log.read_text(encoding="utf-8", errors="replace")
    lines = [line for line in text.splitlines()
             if line.startswith("[recursive-scc-transfer-diagnostic]")]
    if not lines:
        raise SystemExit("missing recursive-SCC transfer diagnostic records")
    required = [
        "scc-id=", "edge=", "callee-param=$", "caller-param=$",
        "origin-guard=", "prefix-guarded-join=", "prefix-ptrarith=",
        "callee-core=", "candidate=", "stored-before=", "stored-after=",
        "summary-changed=", "change-class=",
    ]
    for token in required:
        if not any(token in line for line in lines):
            raise SystemExit(f"missing diagnostic field: {token}")
    if args.fixture == "p127":
        if not any("edge=\"f -> g\"" in line or "edge=\"g -> f\"" in line
                   for line in lines):
            raise SystemExit("p127 log has no f <-> g recursive transfer")
        if not any("prefix-ptrarith=yes" in line for line in lines):
            raise SystemExit("p127 log did not preserve pointer-arithmetic evidence")
    else:
        if not any("edge=\"relay_a -> relay_b\"" in line or
                   "edge=\"relay_b -> relay_a\"" in line for line in lines):
            raise SystemExit("p126 log has no relay_a <-> relay_b transfer")
    print(f"ok: {args.fixture}; records={len(lines)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

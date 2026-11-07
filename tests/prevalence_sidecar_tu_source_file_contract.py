#!/usr/bin/env python3
"""Contract test: a prevalence sidecar must be valid UTF-8 and name its TU.

`CastHistoryVisitor::emitPrevalenceMetricsJson` passes `mainFileNameForSummary()`
-- which returns `std::string` by value -- into an `llvm::StringRef` parameter
that `llvm::json::Value` stores WITHOUT taking ownership. The temporary dies at
the end of the call expression, so `tu.sourceFile` is serialized from freed
memory:

    "sourceFile": "\\u0000-\\u001eo\\u00ff\\u007f\\u0000..."

Two consequences this test pins down:

  1. The file is no longer valid UTF-8, so the obvious way to read it --
     `json.loads(path.read_text(encoding="utf-8"))`, the pattern
     `scripts/extract_stable_classifications.py` uses -- raises
     UnicodeDecodeError. Every prevalence read site in scripts/ currently
     carries a hand-added `errors="replace"`, which hides the crash but not
     the corruption.

  2. Even read leniently, `tu.sourceFile` is garbage, so a consumer cannot use
     it to attribute a sidecar to its translation unit.

Usage:
    python3 tests/prevalence_sidecar_tu_source_file_contract.py <output-root>

<output-root> is a cast-history runner output directory (it is searched
recursively for *.cast-prevalence.json). Exit status 0 when every sidecar
satisfies the contract, 1 when any does not, 2 on a usage problem.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

# Bytes that never appear in a legitimately emitted source path.
CONTROL = {chr(c) for c in range(0x20)} - {"\t", "\n", "\r"}


def check(path: Path) -> list[str]:
    problems: list[str] = []
    raw = path.read_bytes()

    # (1) the strict read every naive consumer performs
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        problems.append(f"not valid UTF-8: {exc}")
        text = raw.decode("utf-8", "replace")

    try:
        doc = json.loads(text)
    except ValueError as exc:
        problems.append(f"not valid JSON: {exc}")
        return problems

    # (2) the field must actually name the translation unit
    source_file = (doc.get("tu") or {}).get("sourceFile")
    if not isinstance(source_file, str) or not source_file:
        problems.append("tu.sourceFile is missing or empty")
        return problems
    if "�" in source_file:
        problems.append(
            f"tu.sourceFile holds undecodable bytes: {source_file!r}"
        )
    elif any(c in CONTROL for c in source_file):
        problems.append(
            f"tu.sourceFile holds control characters: {source_file!r}"
        )
    elif not source_file.endswith((".c", ".cc", ".cpp", ".cxx", ".h", ".hpp")):
        problems.append(
            f"tu.sourceFile is not a source path: {source_file!r}"
        )
    return problems


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    root = Path(argv[0])
    if not root.exists():
        print(f"error: no such path: {root}", file=sys.stderr)
        return 2

    files = (
        [root]
        if root.is_file()
        else sorted(root.rglob("*.cast-prevalence.json"))
    )
    if not files:
        print(f"error: no *.cast-prevalence.json under {root}", file=sys.stderr)
        print("       run the runner with prevalence metrics enabled", file=sys.stderr)
        return 2

    bad = 0
    for path in files:
        problems = check(path)
        if problems:
            bad += 1
            print(f"FAIL {path}")
            for problem in problems:
                print(f"       {problem}")
        else:
            print(f"ok   {path.name}")

    print()
    print(f"{len(files) - bad}/{len(files)} prevalence sidecar(s) satisfy the contract")
    if bad:
        print(
            "\nFix: in CastHistoryVisitor::emitPrevalenceMetricsJson, hand the "
            "emitter an owning string. `llvm::json::Value`'s StringRef "
            "constructor does not copy, so the std::string returned by "
            "mainFileNameForSummary() must outlive the json::Value -- bind it "
            "to a named local, or change emitJsonObject's parameter from "
            "llvm::StringRef to std::string."
        )
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

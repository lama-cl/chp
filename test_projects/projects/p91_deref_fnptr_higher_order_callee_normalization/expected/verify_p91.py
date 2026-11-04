#!/usr/bin/env python3
"""Verify the C6g4b semantic result for p91 text output.

Usage:
  python3 verify_p91.py /path/to/a.c.txt

The check intentionally targets the resolved caller histories.  It does not
constrain generic invoke_direct/invoke_deref summaries, which must remain
symbolic so the functions can be reused with other callbacks.
"""
from __future__ import annotations

import argparse
from pathlib import Path

KEY = "const int * => const void * => const int *"
BASE = (
    "const int * => const void * => const char * => "
    "ptrarith(const char *) => const void * => const int *"
)


def find_line(text: str, prefix: str) -> str:
    for line in text.splitlines():
        if line.startswith(prefix):
            return line
    raise AssertionError(f"missing line beginning with {prefix!r}")


def require_resolved(line: str, expected: str, label: str) -> None:
    if "$0.$" in line:
        raise AssertionError(f"{label} still contains an unresolved callback reference: {line}")
    if "\u27e8|" in line or "\u27e8[" in line:
        raise AssertionError(f"{label} still contains a non-deduplicated alternative: {line}")
    if expected not in line:
        raise AssertionError(f"{label} is not the expected semantic history:\n{line}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    text = args.output.read_text(encoding="utf-8")

    function_summary = find_line(text, "  exercise(")
    require_resolved(function_summary, KEY, "exercise function summary")
    require_resolved(function_summary, BASE, "exercise function summary")

    key_history = find_line(text, "  exercise::key :")
    base_history = find_line(text, "  exercise::base :")
    require_resolved(key_history, KEY, "exercise::key")
    require_resolved(base_history, BASE, "exercise::base")


if __name__ == "__main__":
    main()

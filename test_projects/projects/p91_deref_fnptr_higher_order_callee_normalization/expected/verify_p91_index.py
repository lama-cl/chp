#!/usr/bin/env python3
"""Verify C6g4c project-index specialization for p91.

Usage:
  python3 verify_p91_index.py /path/to/project-summary-index.json

C6g4c must not reintroduce the generic invoke_$0.$i arm during the C2
project-param-transfer fixpoint after the C6g4b visitor-level specialization
has already produced a concrete C1 summary.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

KEY_TAIL = "const void * => const int *"
BASE_TAIL = (
    "const void * => const char * => ptrarith(const char *) => "
    "const void * => const int *"
)


def exercise_entry(entries: list[dict]) -> dict:
    for entry in entries:
        if entry.get("function") == "exercise":
            return entry
    raise AssertionError("missing exercise entry in project index")


def check(record: dict, expected: str, label: str) -> None:
    rendered = str(record.get("rendered") or "")
    core = str(record.get("semanticCoreRendered") or "")
    for value, field in ((rendered, "rendered"), (core, "semanticCoreRendered")):
        if "$0.$" in value:
            raise AssertionError(f"{label} {field} retains a symbolic callback reference: {value}")
        if "⟨" in value:
            raise AssertionError(f"{label} {field} retains a non-deduplicated alternative: {value}")
        if value != expected:
            raise AssertionError(f"{label} {field} differs from expected:\n{value}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("index_json", type=Path)
    args = parser.parse_args()
    data = json.loads(args.index_json.read_text(encoding="utf-8"))

    exercise = exercise_entry(data.get("functions", []))
    records = {record.get("index"): record
               for record in exercise.get("solvedParamSummaries", [])}
    check(records[0], KEY_TAIL, "exercise.$0")
    check(records[1], BASE_TAIL, "exercise.$1")


if __name__ == "__main__":
    main()

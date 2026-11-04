#!/usr/bin/env python3
"""Verify the C6g4b p91 result in the serialized cast-summary JSON."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

KEY_TAIL = "const void * => const int *"
BASE_TAIL = (
    "const void * => const char * => ptrarith(const char *) => "
    "const void * => const int *"
)
KEY_FULL = "const int * => " + KEY_TAIL
BASE_FULL = "const int * => " + BASE_TAIL


def require_concrete(value: str, expected: str, label: str) -> None:
    if "$0.$" in value:
        raise AssertionError(f"{label} still contains an unresolved callback reference: {value}")
    if "\u27e8" in value:
        raise AssertionError(f"{label} still contains a non-deduplicated alternative: {value}")
    if value != expected:
        raise AssertionError(f"{label} differs from the expected history:\n{value}")


def function_entry(entries: list[dict], name: str) -> dict:
    for entry in entries:
        if entry.get("function") == name:
            return entry
    raise AssertionError(f"missing function summary for {name}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("summary_json", type=Path)
    args = parser.parse_args()
    data = json.loads(args.summary_json.read_text(encoding="utf-8"))

    exercise = function_entry(data["functionSummaries"], "exercise")
    params = {param["index"]: param for param in exercise["params"]}
    require_concrete(params[0]["summary"], KEY_TAIL, "exercise.$0 summary")
    require_concrete(params[1]["summary"], BASE_TAIL, "exercise.$1 summary")
    if params[0].get("summaryIrKind") == "parallel" or params[1].get("summaryIrKind") == "parallel":
        raise AssertionError("exercise summary still serializes as a parallel history")

    histories = {entry["entity"]: entry["history"] for entry in data["directCastHistories"]}
    require_concrete(histories["exercise::key"], KEY_FULL, "exercise::key")
    require_concrete(histories["exercise::base"], BASE_FULL, "exercise::base")

    # Keep the generic callees polymorphic; specialization occurs only at the
    # uniquely resolved direct call from exercise.
    for name in ("invoke_direct", "invoke_deref"):
        invoke = function_entry(data["functionSummaries"], name)
        invoke_params = {param["index"]: param for param in invoke["params"]}
        if "$0.$0" not in invoke_params[1]["summary"]:
            raise AssertionError(f"{name}.$1 lost its generic callback summary")
        if "$0.$1" not in invoke_params[2]["summary"]:
            raise AssertionError(f"{name}.$2 lost its generic callback summary")


if __name__ == "__main__":
    main()

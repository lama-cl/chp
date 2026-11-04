#!/usr/bin/env python3
"""C6g5w1a diagnostics-only branch metadata rehydration smoke test.

The test builds a small project index carrying p130-shaped source-family arms,
then verifies that the C2 companion record remains complete if an import-aware
index rewrite omits branchFamily/branchArm payloads from copied root edges.
No project solver data is inspected or modified.
"""
from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "scripts" / "build_summary_index.py"
INSPECTOR = ROOT / "scripts" / "inspect_summary_index.py"


def load_builder():
    spec = importlib.util.spec_from_file_location("c6g5w1a_builder", BUILDER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def edge(family: str, arm: str, path: str, line: int) -> dict:
    sig = "fn:(ptr:void,int:unsigned int)->void"
    caller = f"p130_relay_a::{sig}"
    callee = f"p130_relay_b::{sig}"
    return {
        "kind": "direct-call-param-transfer",
        "caller": "p130_relay_a",
        "callerQualifiedName": "p130_relay_a",
        "callerSignatureKey": sig,
        "callerNode": caller,
        "callee": "p130_relay_b",
        "calleeQualifiedName": "p130_relay_b",
        "calleeSignatureKey": sig,
        "calleeNode": callee,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"rendered": path, "ir": None},
        "location": {"file": "a.c", "line": line, "column": 3},
        "branchFamily": {
            "id": family,
            "kind": "parallel-call-actuals",
            "ownerNode": caller,
            "ownerParamIndex": 0,
            "origin": "distinct direct-call actual paths from one caller formal to one callee formal",
            # C6g5w1c: p130 keeps exact source-family closure identity.
            "recursiveClosureKey": family,
            "memberCount": 2,
            "diagnosticOnly": True,
        },
        "branchArm": {
            "id": f"{family}:arm@a.c:{line}:3:path={path}",
            "kind": "call-actual-path",
            "familyId": family,
            "actualPath": path,
            "originSourceEntity": "p130_relay_a::p",
            "callSite": {"file": "a.c", "line": line, "column": 3},
            "diagnosticOnly": True,
        },
    }


def main() -> None:
    builder = load_builder()
    family = (
        "branch-family:parallel-call-actuals:"
        "p130_relay_a::fn:(ptr:void,int:unsigned int)->void.$0->"
        "p130_relay_b::fn:(ptr:void,int:unsigned int)->void.$0"
    )
    summary = {
        "schemaName": "cast-history-tu-link-summary",
        "schemaVersion": 1,
        "tu": {"sourceFile": "a.c"},
        "functionSummaryDetails": [],
        "summaryParamTransferEdges": [
            edge(family, "int", "int * => void *", 10),
            edge(family, "char", "char * => void *", 11),
        ],
    }
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        summary_file = root / "a.c.cast-summary.json"
        summary_file.write_text(json.dumps(summary), encoding="utf-8")
        index, issues = builder.build_index([summary_file], root / "index.json", project_kind="real")
        assert not [i for i in issues if i.severity == "error"], issues
        meta = index["branchFamilyTransferMetadata"]
        assert "rehydrationContract" in meta
        assert meta["counts"]["families"] == 1
        assert meta["counts"]["arms"] == 2
        assert meta["families"][0]["recursiveClosureKey"] == family
        assert {a["actualPath"] for a in meta["families"][0]["arms"]} == {
            "int * => void *", "char * => void *"
        }

        # Model an import-aware rewrite retaining the project companion while
        # stripping optional tags from copied raw transfer edges.
        rewritten = json.loads(json.dumps(index))
        for transfer in rewritten.get("summaryParamTransferEdges", []):
            transfer.pop("branchFamily", None)
            transfer.pop("branchArm", None)
        rewritten_file = root / "import-round-index.json"
        rewritten_file.write_text(json.dumps(rewritten), encoding="utf-8")

        # The Python inspector intentionally reads the project companion—the
        # same fallback C6g5w1a makes available to SummaryIndex import.
        spec = importlib.util.spec_from_file_location("c6g5w1a_inspector", INSPECTOR)
        assert spec and spec.loader
        inspector = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = inspector
        spec.loader.exec_module(inspector)
        reloaded = json.loads(rewritten_file.read_text(encoding="utf-8"))
        families = reloaded["branchFamilyTransferMetadata"]["families"]
        assert len(families) == 1
        assert len(families[0]["arms"]) == 2
        assert families[0]["recursiveClosureKey"] == family
        assert "rehydrationContract" in reloaded["branchFamilyTransferMetadata"]

    print("C6g5w1a branch-family rehydration smoke passed")


if __name__ == "__main__":
    main()

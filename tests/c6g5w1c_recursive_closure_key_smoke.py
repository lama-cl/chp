#!/usr/bin/env python3
"""C6g5w1c recursive-closure-key transport smoke test.

Models the metadata-only contract needed before any composition change:
  * p126/p130 parallel-call families use exact source-family identity; and
  * p127 f/g guarded-local families keep different provenance family IDs but
    share one canonical recursiveClosureKey.

The test invokes only C2 index construction.  It does not exercise project
summary solving or alter any solved history.
"""
from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "scripts" / "build_summary_index.py"


def load_builder():
    spec = importlib.util.spec_from_file_location("c6g5w1c_builder", BUILDER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def parallel_edge(sig: str, family: str, arm_id: str, actual: str,
                  line: int) -> dict:
    node_a = f"relay_a::{sig}"
    node_b = f"relay_b::{sig}"
    return {
        "kind": "direct-call-param-transfer",
        "caller": "relay_a",
        "callerQualifiedName": "relay_a",
        "callerSignatureKey": sig,
        "callerNode": node_a,
        "callee": "relay_b",
        "calleeQualifiedName": "relay_b",
        "calleeSignatureKey": sig,
        "calleeNode": node_b,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"rendered": actual, "ir": None},
        "location": {"file": "a.c", "line": line, "column": 3},
        "branchFamily": {
            "id": family,
            "kind": "parallel-call-actuals",
            "ownerNode": node_a,
            "ownerParamIndex": 0,
            "origin": "distinct direct-call actual paths from one caller formal to one callee formal",
            "recursiveClosureKey": family,
            "memberCount": 2,
            "diagnosticOnly": True,
        },
        "branchArm": {
            "id": arm_id,
            "kind": "call-actual-path",
            "familyId": family,
            "actualPath": actual,
            "originSourceEntity": "relay_a::p",
            "callSite": {"file": "a.c", "line": line, "column": 3},
            "diagnosticOnly": True,
        },
    }


def guarded_edge(caller: str, callee: str, sig: str, family: str,
                 closure: str, line: int) -> dict:
    caller_node = f"{caller}::{sig}"
    callee_node = f"{callee}::{sig}"
    return {
        "kind": "direct-call-param-transfer",
        "caller": caller,
        "callerQualifiedName": caller,
        "callerSignatureKey": sig,
        "callerNode": caller_node,
        "callee": callee,
        "calleeQualifiedName": callee,
        "calleeSignatureKey": sig,
        "calleeNode": callee_node,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"rendered": "ptrarith(unsigned char *)", "ir": None},
        "location": {"file": "a.c", "line": line, "column": 3},
        "branchFamily": {
            "id": family,
            "kind": "guarded-local-ptrarith",
            "ownerNode": caller_node,
            "ownerParamIndex": 0,
            "origin": "guarded local pointer-arithmetic alternatives for one call actual",
            "recursiveClosureKey": closure,
            "memberCount": 2,
            "diagnosticOnly": True,
        },
        "branchArms": [
            {
                "id": f"{family}:tag",
                "kind": "guarded-local-ptrarith",
                "familyId": family,
                "actualPath": "ptrarith(unsigned char *)",
                "guardText": "tag",
                "originSourceEntity": f"{caller}::p",
                "callSite": {"file": "a.c", "line": line - 3, "column": 11},
                "diagnosticOnly": True,
            },
            {
                "id": f"{family}:not-tag",
                "kind": "guarded-local-ptrarith",
                "familyId": family,
                "actualPath": "ptrarith(unsigned char *)",
                "guardText": "!(tag)",
                "originSourceEntity": f"{caller}::p",
                "callSite": {"file": "a.c", "line": line - 1, "column": 11},
                "diagnosticOnly": True,
            },
        ],
    }


def main() -> None:
    builder = load_builder()
    sig_void = "fn:(ptr:void,int:unsigned int)->void"
    relay_a = f"relay_a::{sig_void}"
    relay_b = f"relay_b::{sig_void}"
    parallel_family = (
        "branch-family:parallel-call-actuals:"
        f"{relay_a}.$0->{relay_b}.$0"
    )
    sig_byte = "fn:(ptr:byte,int:int)->void"
    f_node = f"f::{sig_byte}"
    g_node = f"g::{sig_byte}"
    f_family = (
        "branch-family:guarded-local-ptrarith:"
        f"{f_node}.$0->{g_node}.$0@a.c:19:3"
    )
    g_family = (
        "branch-family:guarded-local-ptrarith:"
        f"{g_node}.$0->{f_node}.$0@a.c:28:3"
    )
    guarded_key = (
        "recursive-closure:guarded-local-ptrarith:formal=$0:"
        "type=unsigned char *:arms=[!(tag)=>ptrarith(unsigned char *),"
        "tag=>ptrarith(unsigned char *)]"
    )
    summary = {
        "schemaName": "cast-history-tu-link-summary",
        "schemaVersion": 1,
        "tu": {"sourceFile": "a.c"},
        "functionSummaryDetails": [],
        "summaryParamTransferEdges": [
            parallel_edge(sig_void, parallel_family,
                          parallel_family + ":int", "int * => void *", 18),
            parallel_edge(sig_void, parallel_family,
                          parallel_family + ":char", "char * => void *", 19),
            guarded_edge("f", "g", sig_byte, f_family, guarded_key, 19),
            guarded_edge("g", "f", sig_byte, g_family, guarded_key, 28),
        ],
    }
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        summary_file = root / "a.c.cast-summary.json"
        summary_file.write_text(json.dumps(summary), encoding="utf-8")
        index, issues = builder.build_index(
            [summary_file], root / "project-summary-index.json", project_kind="real"
        )
        assert not [i for i in issues if i.severity == "error"], issues
        meta = index["branchFamilyTransferMetadata"]
        families = {rec["id"]: rec for rec in meta["families"]}
        assert families[parallel_family]["recursiveClosureKey"] == parallel_family
        assert f_family != g_family
        assert families[f_family]["recursiveClosureKey"] == guarded_key
        assert families[g_family]["recursiveClosureKey"] == guarded_key
        assert len(index["summaryParamTransferEdges"]) == 4
        assert "recursiveClosureKey" not in index.get("solvedParamSummaries", {})

    print("C6g5w1c recursive closure-key transport smoke passed")


if __name__ == "__main__":
    main()

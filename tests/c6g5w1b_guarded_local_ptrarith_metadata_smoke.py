#!/usr/bin/env python3
"""C6g5w1b guarded-local ptrarith metadata smoke test.

Models the p127 export shape: one ordinary f.$0 -> g.$0 transfer edge remains
one edge, but carries two diagnostics-only branch arms extracted from local q
assignments under tag / !(tag).  The test exercises only C2 metadata indexing;
it does not invoke or alter project summary solving.
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
    spec = importlib.util.spec_from_file_location("c6g5w1b_builder", BUILDER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main() -> None:
    builder = load_builder()
    sig = "fn:(ptr:byte,int:int)->void"
    caller = f"f::{sig}"
    callee = f"g::{sig}"
    family = (
        "branch-family:guarded-local-ptrarith:"
        f"{caller}.$0->{callee}.$0@a.c:19:3"
    )
    edge = {
        "kind": "direct-call-param-transfer",
        "caller": "f",
        "callerQualifiedName": "f",
        "callerSignatureKey": sig,
        "callerNode": caller,
        "callee": "g",
        "calleeQualifiedName": "g",
        "calleeSignatureKey": sig,
        "calleeNode": callee,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"rendered": "ptrarith(unsigned char *)", "ir": None},
        "location": {"file": "a.c", "line": 19, "column": 3},
        "branchFamily": {
            "id": family,
            "kind": "guarded-local-ptrarith",
            "ownerNode": caller,
            "ownerParamIndex": 0,
            "origin": "guarded local pointer-arithmetic alternatives for one call actual",
            "memberCount": 2,
            "diagnosticOnly": True,
        },
        "branchArms": [
            {
                "id": f"{family}:arm@a.c:14:7:path=ptrarith(unsigned char *):guard=tag",
                "kind": "guarded-local-ptrarith",
                "familyId": family,
                "actualPath": "ptrarith(unsigned char *)",
                "guardText": "tag",
                "originSourceEntity": "f::p",
                "callSite": {"file": "a.c", "line": 14, "column": 7},
                "diagnosticOnly": True,
            },
            {
                "id": f"{family}:arm@a.c:16:7:path=ptrarith(unsigned char *):guard=!(tag)",
                "kind": "guarded-local-ptrarith",
                "familyId": family,
                "actualPath": "ptrarith(unsigned char *)",
                "guardText": "!(tag)",
                "originSourceEntity": "f::p",
                "callSite": {"file": "a.c", "line": 16, "column": 7},
                "diagnosticOnly": True,
            },
        ],
    }
    summary = {
        "schemaName": "cast-history-tu-link-summary",
        "schemaVersion": 1,
        "tu": {"sourceFile": "a.c"},
        "functionSummaryDetails": [],
        "summaryParamTransferEdges": [edge],
    }

    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        summary_file = root / "a.c.cast-summary.json"
        summary_file.write_text(json.dumps(summary), encoding="utf-8")
        index, issues = builder.build_index([summary_file], root / "index.json", project_kind="real")
        assert not [i for i in issues if i.severity == "error"], issues
        assert len(index["summaryParamTransferEdges"]) == 1
        meta = index["branchFamilyTransferMetadata"]
        assert meta["counts"]["families"] == 1
        assert meta["counts"]["arms"] == 2
        arms = meta["families"][0]["arms"]
        assert {a["guardText"] for a in arms} == {"tag", "!(tag)"}
        assert {a["actualPath"] for a in arms} == {"ptrarith(unsigned char *)"}
        assert {a["originSourceEntity"] for a in arms} == {"f::p"}

    print("C6g5w1b guarded-local ptrarith metadata smoke passed")


if __name__ == "__main__":
    main()

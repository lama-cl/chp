#!/usr/bin/env python3
"""C6g5w1 metadata-only branch-family transport smoke test.

This test drives the ordinary raw-TU -> C2 project-index path with synthetic
p126/p130-shaped transfer edges.  It intentionally does not invoke, inspect,
or mutate the project parameter solver: C6g5w1 must be transport-only.
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
    spec = importlib.util.spec_from_file_location("c6g5w1_builder", BUILDER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def transfer_edge(*, family: str, arm: str, path: str, line: int,
                  caller: str, callee: str, source: str,
                  kind: str = "parallel-call-actuals") -> dict:
    sig = "fn:(ptr:void,int:unsigned int)->void"
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
        "actualPathFromCallerParam": {"rendered": path, "ir": None},
        "location": {"file": source, "line": line, "column": 3},
        "branchFamily": {
            "id": family,
            "kind": kind,
            "ownerNode": caller_node,
            "ownerParamIndex": 0,
            "origin": "synthetic source-defined branch family",
            "memberCount": 2,
            "diagnosticOnly": True,
        },
        "branchArm": {
            "id": f"{family}:arm@{source}:{line}:3:path={path}",
            "kind": "call-actual-path",
            "familyId": family,
            "actualPath": path,
            "originSourceEntity": f"{caller}::p",
            "callSite": {"file": source, "line": line, "column": 3},
            "diagnosticOnly": True,
        },
    }


def tu_summary(source: str, transfers: list[dict]) -> dict:
    return {
        "schemaName": "cast-history-tu-link-summary",
        "schemaVersion": 1,
        "tu": {"sourceFile": source},
        # No function details are needed for this transport-only smoke.
        "functionSummaryDetails": [],
        "summaryParamTransferEdges": transfers,
    }


def build_index(builder, summaries: list[dict]) -> dict:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        files: list[Path] = []
        for i, summary in enumerate(summaries):
            path = root / f"tu-{i}.cast-summary.json"
            path.write_text(json.dumps(summary), encoding="utf-8")
            files.append(path)
        index, issues = builder.build_index(files, root / "project-summary-index.json",
                                            project_kind="real")
        # Empty functionSummaryDetails only produces benign fixture warnings.
        assert not [x for x in issues if x.severity == "error"], issues
        return index


def assert_family(index: dict, family: str, expected_paths: set[str],
                  expected_kind: str = "parallel-call-actuals",
                  expected_arm_count: int | None = None) -> None:
    meta = index.get("branchFamilyTransferMetadata", {})
    assert meta.get("counts", {}).get("families") == 1, meta
    if expected_arm_count is None:
        expected_arm_count = len(expected_paths)
    assert meta.get("counts", {}).get("arms") == expected_arm_count, meta
    records = meta.get("families", [])
    assert len(records) == 1, records
    rec = records[0]
    assert rec.get("id") == family, rec
    assert rec.get("kind") == expected_kind, rec
    paths = {str(a.get("actualPath")) for a in rec.get("arms", [])}
    assert paths == expected_paths, (paths, expected_paths)
    # The same metadata remains attached to the ordinary transfer edges.
    edge_paths = {
        str(e.get("branchArm", {}).get("actualPath"))
        for e in index.get("summaryParamTransferEdges", [])
        if isinstance(e, dict) and isinstance(e.get("branchArm"), dict)
    }
    assert edge_paths == expected_paths, edge_paths
    # This metadata must not be materialized into authoritative solved facts.
    assert all("branchFamily" not in rec
               for entry in index.get("functions", [])
               for rec in entry.get("solvedParamSummaries", [])
               if isinstance(entry, dict) and isinstance(rec, dict))


def main() -> None:
    builder = load_builder()

    # Same-TU p126 shape.
    f126 = (
        "branch-family:parallel-call-actuals:"
        "relay_a::fn:(ptr:void,int:unsigned int)->void.$0->"
        "relay_b::fn:(ptr:void,int:unsigned int)->void.$0"
    )
    p126 = build_index(builder, [tu_summary("a.c", [
        transfer_edge(family=f126, arm="int", path="int * => void *", line=18,
                      caller="relay_a", callee="relay_b", source="a.c"),
        transfer_edge(family=f126, arm="char", path="char * => void *", line=19,
                      caller="relay_a", callee="relay_b", source="a.c"),
    ])])
    assert_family(p126, f126, {"int * => void *", "char * => void *"})

    # p127 shape: one call site with two guarded reaching-origin alternatives.
    # The C++ emitter calls this a guarded-origin-alternatives family; the
    # guards distinguish its arms even though their PtrArith path shape matches.
    f127 = (
        "branch-family:guarded-origin-alternatives:"
        "f::fn:(ptr:byte,int:int)->void.$0->"
        "g::fn:(ptr:byte,int:int)->void.$0@a.c:19:3"
    )
    sig127 = "fn:(ptr:byte,int:int)->void"
    def p127_edge(guard: str) -> dict:
        caller_node = f"f::{sig127}"
        callee_node = f"g::{sig127}"
        return {
            "kind": "direct-call-param-transfer",
            "caller": "f", "callerQualifiedName": "f",
            "callerSignatureKey": sig127, "callerNode": caller_node,
            "callee": "g", "calleeQualifiedName": "g",
            "calleeSignatureKey": sig127, "calleeNode": callee_node,
            "callerParamIndex": 0, "calleeParamIndex": 0,
            "actualPathFromCallerParam": {"rendered": "ptrarith(unsigned char *)", "ir": None},
            "guard": {"text": guard, "kind": "branch"},
            "location": {"file": "a.c", "line": 19, "column": 3},
            "branchFamily": {
                "id": f127, "kind": "guarded-origin-alternatives",
                "ownerNode": caller_node, "ownerParamIndex": 0,
                "origin": "synthetic guarded reaching origins",
                "memberCount": 2, "diagnosticOnly": True,
            },
            "branchArm": {
                "id": f"{f127}:arm:{guard}", "kind": "origin-alternative",
                "familyId": f127, "actualPath": "ptrarith(unsigned char *)",
                "originSourceEntity": "f::p",
                "callSite": {"file": "a.c", "line": 19, "column": 3},
                "diagnosticOnly": True,
            },
        }
    p127 = build_index(builder, [tu_summary("a.c", [p127_edge("tag"), p127_edge("!(tag)")])])
    assert_family(p127, f127, {"ptrarith(unsigned char *)"},
                  expected_kind="guarded-origin-alternatives",
                  expected_arm_count=2)

    # Cross-TU p130 shape.  The family is emitted by a.c, travels through the
    # same project index used by b.c, and stays attached to normal transfer
    # edges rather than a canonical-report companion.
    f130 = (
        "branch-family:parallel-call-actuals:"
        "p130_relay_a::fn:(ptr:void,int:unsigned int)->void.$0->"
        "p130_relay_b::fn:(ptr:void,int:unsigned int)->void.$0"
    )
    p130 = build_index(builder, [
        tu_summary("a.c", [
            transfer_edge(family=f130, arm="int", path="int * => void *", line=10,
                          caller="p130_relay_a", callee="p130_relay_b", source="a.c"),
            transfer_edge(family=f130, arm="char", path="char * => void *", line=11,
                          caller="p130_relay_a", callee="p130_relay_b", source="a.c"),
        ]),
        tu_summary("b.c", []),
    ])
    assert_family(p130, f130, {"int * => void *", "char * => void *"})

    print("C6g5w1 branch-family transport smoke passed")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Smoke-test C6g5s local/imported SCC topology metadata."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module_path = root / "scripts" / "build_summary_index.py"
spec = importlib.util.spec_from_file_location("c6g5s_build_summary_index", module_path)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def edge(src: str, dst: str, *, imported: bool, tu: str) -> dict[str, object]:
    return {
        "callerNode": src,
        "calleeNode": dst,
        "callerSignatureKey": "void(void*,int)",
        "calleeSignatureKey": "void(void*,int)",
        "crossTUCandidate": imported,
        "tuSourceFile": tu,
    }


def one_recursive_component(report: dict[str, object]) -> dict[str, object]:
    components = report["components"]
    recursive = [c for c in components if c["recursive"]]
    assert len(recursive) == 1, recursive
    return recursive[0]

local = module.build_scc_report(
    [edge("f::sig", "g::sig", imported=False, tu="a.c"),
     edge("g::sig", "f::sig", imported=False, tu="a.c")],
    concrete_only=True,
)
local_component = one_recursive_component(local)
assert local["edgeTopologyVersion"] == 1
assert local_component["concreteEdgeCount"] == 2
assert local_component["localEdgeCount"] == 2
assert local_component["importedEdgeCount"] == 0
assert local_component["translationUnits"] == ["a.c"]

cross = module.build_scc_report(
    [edge("f::sig", "g::sig", imported=True, tu="f.c"),
     edge("g::sig", "f::sig", imported=True, tu="g.c")],
    concrete_only=True,
)
cross_component = one_recursive_component(cross)
assert cross_component["concreteEdgeCount"] == 2
assert cross_component["localEdgeCount"] == 0
assert cross_component["importedEdgeCount"] == 2
assert cross_component["translationUnits"] == ["f.c", "g.c"]

mixed = module.build_scc_report(
    [edge("f::sig", "g::sig", imported=False, tu="f.c"),
     edge("g::sig", "h::sig", imported=True, tu="g.c"),
     edge("h::sig", "f::sig", imported=True, tu="h.c")],
    concrete_only=True,
)
mixed_component = one_recursive_component(mixed)
assert mixed_component["localEdgeCount"] == 1
assert mixed_component["importedEdgeCount"] == 2

print("C6g5s summary-index SCC topology smoke: OK")

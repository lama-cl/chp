#!/usr/bin/env python3
"""C6g5u smoke: direct-local bases, frozen callee steps, and forwarders."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module_path = root / "scripts" / "build_summary_index.py"
spec = importlib.util.spec_from_file_location("c6g5u_build_summary_index", module_path)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def ty(text: str) -> dict[str, object]:
    return {
        "kind": "asType",
        "type": {"text": text, "canonical": text, "isPointer": text.endswith("*")},
    }


def seq(*items: dict[str, object] | None) -> dict[str, object] | None:
    out: dict[str, object] | None = None
    for item in items:
        out = module.compose_ir(out, item)
    return out


def parallel(*items: dict[str, object]) -> dict[str, object]:
    return module.normalize_ir({"kind": "parallel", "arms": list(items)})


def summary(ir: dict[str, object] | None) -> dict[str, object]:
    return {"ir": ir, "rendered": module.render_ir(ir)}


def edge(caller: str, callee: str, path: dict[str, object] | None) -> dict[str, object]:
    return {
        "kind": "direct-call",
        "callerNode": caller,
        "calleeNode": callee,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"ir": path},
        "guard": None,
    }


def entry(node: str) -> dict[str, object]:
    return {
        "function": node,
        "qualifiedName": node,
        "signatureKey": "sig",
        "publicLookupKey": node,
        "lookupKey": node,
        "linkage": "external",
        "sourceFile": f"{node}.c",
        "summaryFile": f"{node}.cast-summary.json",
        "resolution": "safe",
    }


def detail(node: str, raw: dict[str, object] | None,
           direct: dict[str, object] | None) -> dict[str, object]:
    return {
        "function": node,
        "params": [{
            "index": 0,
            "declaredType": "void *",
            "summary": summary(raw),
            # Presence with ir=None is significant: this is an identity
            # forwarder, not a legacy summary that lacks the new field.
            "directLocalSummary": summary(direct),
        }],
    }

# p126-style local fork: append relay_b's frozen short* base while preserving
# the int*/char* root arm identity.
a = "relay_a::sig"
b = "relay_b::sig"
solved = {
    a: {0: {
        "index": 0,
        "ir": parallel(ty("int *"), ty("char *")),
        "projectBaseIr": parallel(ty("int *"), ty("char *")),
        "directLocalBasePresent": True,
        "projectTrace": [],
    }},
    b: {0: {
        "index": 0,
        "ir": ty("short *"),
        "projectBaseIr": ty("short *"),
        "directLocalBasePresent": True,
        "projectTrace": [],
    }},
}
module.solve_recursive_scc_by_simple_paths(
    solved,
    [
        edge(a, b, seq(ty("int *"), ty("void *"))),
        edge(a, b, seq(ty("char *"), ty("void *"))),
        edge(b, a, seq(ty("short *"), ty("void *"))),
    ],
    {a, b}, {a: 0, b: 0},
    algebra=module.ProjectSummaryAlgebra.from_env(),
    known_param_counts={a: 1, b: 1},
)
a_rendered = module.render_ir(solved[a][0]["ir"])
assert "int * => void * => short * => void *" in a_rendered, a_rendered
assert "char * => void * => short * => void *" in a_rendered, a_rendered

# p23/p129-style ordinary recursive cycle: direct local bases must be appended
# exactly once per simple visit, not replaced by the aggregate report summary.
f = "f::sig"
relay = "relay::sig"
g = "g::sig"
cycle = {
    f: {0: {"index": 0, "ir": ty("char *"), "projectBaseIr": ty("char *"),
            "directLocalBasePresent": True, "projectTrace": []}},
    relay: {0: {"index": 0, "ir": None, "projectBaseIr": None,
                "directLocalBasePresent": True, "projectTrace": []}},
    g: {0: {"index": 0, "ir": ty("int *"), "projectBaseIr": ty("int *"),
            "directLocalBasePresent": True, "projectTrace": []}},
}
module.solve_recursive_scc_by_simple_paths(
    cycle,
    [
        edge(f, relay, seq(ty("char *"), ty("void *"))),
        edge(relay, g, None),
        edge(g, f, seq(ty("int *"), ty("void *"))),
    ],
    {f, relay, g}, {f: 0, relay: 0, g: 0},
    algebra=module.ProjectSummaryAlgebra.from_env(),
    known_param_counts={f: 1, relay: 1, g: 1},
)
assert "char * => void * => int * => void *" in module.render_ir(cycle[f][0]["ir"])
assert "int * => void * => char * => void *" in module.render_ir(cycle[relay][0]["ir"])

# A forwarder with no direct local base must start empty even when its raw C1
# report retained an imported call wrapper. The C2 transfer installs the
# canonical callee fact instead of joining that stale report tree.
forward = "run::sig"
entries = [entry(a), entry(b), entry(forward)]
details = {
    a: detail(a, parallel(ty("int *"), ty("char *")), parallel(ty("int *"), ty("char *"))),
    b: detail(b, ty("short *"), ty("short *")),
    forward: detail(forward, seq(ty("legacy-imported *"), ty("stale *")), None),
}
dep_sccs = {
    "concreteFunctionGraph": {
        "components": [{"id": 0, "recursive": True, "members": [a, b]}],
        "nodeToScc": {a: 0, b: 0, forward: 1},
    },
}
module.materialize_project_solved_param_summaries(
    entries, details, dep_sccs,
    [
        edge(a, b, seq(ty("int *"), ty("void *"))),
        edge(a, b, seq(ty("char *"), ty("void *"))),
        edge(b, a, seq(ty("short *"), ty("void *"))),
        edge(forward, a, None),
    ],
)
solved_by_name = {str(e["function"]): e.get("solvedParamSummaries", []) for e in entries}
a_solved = solved_by_name[a][0]["rendered"]
forward_solved = solved_by_name[forward][0]["rendered"]
assert forward_solved == a_solved, (forward_solved, a_solved)
assert "legacy-imported" not in forward_solved, forward_solved

print("C6g5u direct-local project materialization smoke: OK")

#!/usr/bin/env python3
"""C6g5w1d smoke: consume closure metadata only in tagged C2 SCC summaries."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "scripts" / "build_summary_index.py"
spec = importlib.util.spec_from_file_location("c6g5w1d_builder", BUILDER)
assert spec and spec.loader
m = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = m
spec.loader.exec_module(m)


def ty(text: str) -> dict[str, object]:
    return {"kind": "asType", "type": {"text": text, "canonical": text,
            "isPointer": text.endswith("*")}}


def ptrarith(text: str) -> dict[str, object]:
    return {"kind": "ptrarith", "type": {"text": text, "canonical": text,
            "isPointer": text.endswith("*")}}


def seq(*items: dict[str, object] | None) -> dict[str, object] | None:
    out = None
    for item in items:
        out = m.compose_ir(out, item)
    return out


def parallel(*items: dict[str, object]) -> dict[str, object]:
    return m.normalize_ir({"kind": "parallel", "arms": list(items)})


def guarded() -> dict[str, object]:
    return m.normalize_ir({"kind": "join", "alternatives": [
        {"guard": {"text": "tag", "kind": "if"},
         "history": ptrarith("unsigned char *")},
        {"guard": {"text": "!(tag)", "kind": "if"},
         "history": ptrarith("unsigned char *")},
    ]})


def summary(ir):
    return {"ir": ir, "rendered": m.render_ir(ir)}


def entry(node: str) -> dict[str, object]:
    return {"function": node, "qualifiedName": node, "signatureKey": "sig",
            "publicLookupKey": node, "lookupKey": node, "linkage": "external",
            "sourceFile": f"{node}.c", "summaryFile": f"{node}.cast-summary.json",
            "resolution": "safe"}


def detail(node: str, direct):
    return {"function": node, "params": [{"index": 0, "declaredType": "void *",
            # Deliberately polluted report summary: C6g5w1d must use direct local base.
            "summary": summary(seq(direct, ty("stale recursive *"))),
            "directLocalSummary": summary(direct)}]}


def tagged_edge(caller: str, callee: str, path, family: str, closure: str,
                kind: str) -> dict[str, object]:
    return {"kind": "direct-call-param-transfer", "callerNode": caller,
            "calleeNode": callee, "callerParamIndex": 0, "calleeParamIndex": 0,
            "actualPathFromCallerParam": {"ir": path}, "guard": None,
            "branchFamily": {"id": family, "kind": kind,
                             "recursiveClosureKey": closure}}


def edge(caller: str, callee: str, path) -> dict[str, object]:
    return {"kind": "direct-call-param-transfer", "callerNode": caller,
            "calleeNode": callee, "callerParamIndex": 0, "calleeParamIndex": 0,
            "actualPathFromCallerParam": {"ir": path}, "guard": None}


def scc(*nodes: str) -> dict[str, object]:
    return {"concreteFunctionGraph": {"components": [
        {"id": 0, "recursive": True, "members": list(nodes)}],
        "nodeToScc": {n: 0 for n in nodes}}}


def solved_by(entries):
    out = {}
    for e in entries:
        recs = e.get("solvedParamSummaries", [])
        if recs:
            out[e["publicLookupKey"]] = recs[0]
    return out


# p126/p130 shape: two source arms share one exact closure key.
a, b, run = "relay_a::sig", "relay_b::sig", "run::sig"
family = "branch-family:parallel-call-actuals:relay_a.$0->relay_b.$0"
entries = [entry(a), entry(b), entry(run)]
details = {a: detail(a, parallel(ty("int *"), ty("char *"))),
           b: detail(b, ty("short *")),
           run: detail(run, None)}
edges = [
    tagged_edge(a, b, seq(ty("int *"), ty("void *")), family, family,
                "parallel-call-actuals"),
    tagged_edge(a, b, seq(ty("char *"), ty("void *")), family, family,
                "parallel-call-actuals"),
    edge(b, a, seq(ty("short *"), ty("void *"))),
    edge(run, a, None),
]
stats = m.materialize_project_solved_param_summaries(entries, details, scc(a, b), edges)
solved = solved_by(entries)
assert m.render_ir(solved[a]["ir"]) == (
    "⟨| char * => void * => short * => void *, "
    "int * => void * => short * => void * |⟩"), m.render_ir(solved[a]["ir"])
assert m.render_ir(solved[b]["ir"]) == (
    "⟨| short * => void * => char * => void *, "
    "short * => void * => int * => void * |⟩"), m.render_ir(solved[b]["ir"])
assert m.render_ir(solved[run]["ir"]) == m.render_ir(solved[a]["ir"])
assert solved[a]["source"] == "tagged-recursive-closure-key-simple-scc-path"
assert stats["recursiveClosureKey"]["consumedFamilyIds"] == [family]
assert all(e.get("branchFamily", {}).get("consumedByTaggedRecursiveSolver")
           for e in edges[:2])

# p127 shape: different source family IDs but one canonical closure key.
f, g, forward = "f::sig", "g::sig", "forward::sig"
closure = ("recursive-closure:guarded-local-ptrarith:formal=$0:"
           "type=unsigned char *:arms=[!(tag)=>ptrarith(unsigned char *),"
           "tag=>ptrarith(unsigned char *)]")
f_family = "branch-family:guarded-local-ptrarith:f.$0->g.$0"
g_family = "branch-family:guarded-local-ptrarith:g.$0->f.$0"
entries = [entry(f), entry(g), entry(forward)]
details = {f: detail(f, guarded()), g: detail(g, guarded()), forward: detail(forward, None)}
edges = [
    tagged_edge(f, g, ptrarith("unsigned char *"), f_family, closure,
                "guarded-local-ptrarith"),
    tagged_edge(g, f, ptrarith("unsigned char *"), g_family, closure,
                "guarded-local-ptrarith"),
    edge(forward, f, None),
]
stats = m.materialize_project_solved_param_summaries(entries, details, scc(f, g), edges)
solved = solved_by(entries)
expected = "⟨[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)⟩"
assert m.render_ir(solved[f]["ir"]) == expected
assert m.render_ir(solved[g]["ir"]) == expected
assert m.render_ir(solved[forward]["ir"]) == expected
assert stats["recursiveClosureKey"]["seededGuardedTransformerClosures"] == 2
assert stats["recursiveClosureKey"]["consumedFamilyIds"] == sorted([f_family, g_family])

# Untagged p23-like SCC stays on C6g5s's existing path; no tagged metadata state.
x, relay, y = "x::sig", "relay::sig", "y::sig"
entries = [entry(x), entry(relay), entry(y)]
details = {x: detail(x, ty("char *")), relay: detail(relay, None), y: detail(y, ty("int *"))}
edges = [edge(x, relay, seq(ty("char *"), ty("void *"))),
         edge(relay, y, None), edge(y, x, seq(ty("int *"), ty("void *")))]
stats = m.materialize_project_solved_param_summaries(entries, details, scc(x, relay, y), edges)
assert stats["recursiveClosureKey"]["enabled"] is False
for e in entries:
    for rec in e.get("solvedParamSummaries", []):
        assert rec.get("source") != "tagged-recursive-closure-key-simple-scc-path"

print("C6g5w1d tagged recursive closure solver smoke passed")

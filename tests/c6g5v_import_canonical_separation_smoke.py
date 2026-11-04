#!/usr/bin/env python3
"""C6g5v smoke: import facts and canonical display histories are separate."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module_path = root / "scripts" / "build_summary_index.py"
spec = importlib.util.spec_from_file_location("c6g5v_build_summary_index", module_path)
assert spec and spec.loader
m = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = m
spec.loader.exec_module(m)


def ty(text: str) -> dict[str, object]:
    return {
        "kind": "asType",
        "type": {"text": text, "canonical": text, "isPointer": text.endswith("*")},
    }


def ptrarith(text: str) -> dict[str, object]:
    return {
        "kind": "ptrarith",
        "type": {"text": text, "canonical": text, "isPointer": text.endswith("*")},
    }


def seq(*items: dict[str, object] | None) -> dict[str, object] | None:
    out: dict[str, object] | None = None
    for item in items:
        out = m.compose_ir(out, item)
    return out


def parallel(*items: dict[str, object]) -> dict[str, object]:
    return m.normalize_ir({"kind": "parallel", "arms": list(items)})


def guarded_ptrarith() -> dict[str, object]:
    return m.normalize_ir({
        "kind": "join",
        "alternatives": [
            {"guard": {"text": "tag", "kind": "if"}, "history": ptrarith("unsigned char *")},
            {"guard": {"text": "!(tag)", "kind": "if"}, "history": ptrarith("unsigned char *")},
        ],
    })


def summary(ir: dict[str, object] | None) -> dict[str, object]:
    return {"ir": ir, "rendered": m.render_ir(ir)}


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
            # Presence with a null IR intentionally marks an identity forwarder.
            "directLocalSummary": summary(direct),
        }],
    }


def edge(caller: str, callee: str, path: dict[str, object] | None,
         guard: dict[str, object] | None = None) -> dict[str, object]:
    return {
        "kind": "direct-call",
        "callerNode": caller,
        "calleeNode": callee,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"ir": path},
        "guard": guard,
    }


def dep_sccs(*members: str) -> dict[str, object]:
    recursive = list(members)
    node_to_scc = {name: 0 for name in recursive}
    return {
        "concreteFunctionGraph": {
            "components": [{"id": 0, "recursive": True, "members": recursive}],
            "nodeToScc": node_to_scc,
        }
    }


def canonical_by_name(entries: list[dict[str, object]]) -> dict[str, str]:
    out: dict[str, str] = {}
    for e in entries:
        records = e.get("canonicalProjectParamHistories", [])
        if records:
            out[str(e["publicLookupKey"])] = str(records[0]["rendered"])
    return out


# p126: fork roots are distinct and no arm re-enters relay_a's local fork.
a, b, run = "relay_a::sig", "relay_b::sig", "run::sig"
entries = [entry(a), entry(b), entry(run)]
details = {
    a: detail(a, parallel(ty("int *"), ty("char *")), parallel(ty("int *"), ty("char *"))),
    b: detail(b, ty("short *"), ty("short *")),
    run: detail(run, seq(ty("stale imported *")), None),
}
edges = [
    edge(a, b, seq(ty("int *"), ty("void *"))),
    edge(a, b, seq(ty("char *"), ty("void *"))),
    edge(b, a, seq(ty("short *"), ty("void *"))),
    edge(run, a, None),
]
stats = m.materialize_project_solved_param_summaries(
    entries, details, dep_sccs(a, b), edges)
canon = canonical_by_name(entries)
assert canon[a] == "⟨| char * => void * => short * => void *, int * => void * => short * => void * |⟩", canon[a]
assert canon[b] == "⟨| short * => void * => char * => void *, short * => void * => int * => void * |⟩", canon[b]
assert canon[run] == canon[a], (canon[run], canon[a])
assert entries[0].get("solvedParamSummaries") is not None
assert entries[0].get("canonicalProjectParamHistories") is not None
assert stats["canonicalDisplay"]["note"].startswith("display-only")

# p100/p101: guarded recursive call edges do not become history refinements.
a, b, wrapper = "a::sig", "b::sig", "wrapper::sig"
entries = [entry(a), entry(b), entry(wrapper)]
details = {
    a: detail(a, ty("char *"), ty("char *")),
    b: detail(b, None, None),
    wrapper: detail(wrapper, seq(ty("relay_a *")), None),
}
edges = [
    edge(a, b, None, {"text": "depth > 0", "kind": "if"}),
    edge(b, a, None, {"text": "depth > 0", "kind": "if"}),
    edge(wrapper, a, None),
]
m.materialize_project_solved_param_summaries(entries, details, dep_sccs(a, b), edges)
canon = canonical_by_name(entries)
assert canon[a] == "char *", canon[a]
assert canon[b] == "char *", canon[b]
assert canon[wrapper] == "char *", canon[wrapper]
assert "depth > 0" not in " ".join(canon.values())

# p127: direct guarded PtrArith transformer is retained once and an identity
# forwarder receives the same canonical fact.
f, g, forward = "f::sig", "g::sig", "run::sig"
entries = [entry(f), entry(g), entry(forward)]
transformer = guarded_ptrarith()
details = {
    f: detail(f, transformer, transformer),
    g: detail(g, transformer, transformer),
    forward: detail(forward, seq(ty("legacy *")), None),
}
edges = [edge(f, g, None), edge(g, f, None), edge(forward, f, None)]
m.materialize_project_solved_param_summaries(entries, details, dep_sccs(f, g), edges)
canon = canonical_by_name(entries)
assert canon[f] == canon[g] == canon[forward], canon
assert canon[f] == "⟨[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)⟩", canon[f]

# p23: a connector identity edge must borrow the next direct-local fact;
# separate import summaries remain present and are not replaced by canonical IR.
f, relay, g = "f::sig", "relay::sig", "g::sig"
entries = [entry(f), entry(relay), entry(g)]
details = {
    f: detail(f, ty("char *"), ty("char *")),
    relay: detail(relay, ty("int *"), None),
    g: detail(g, ty("int *"), ty("int *")),
}
edges = [
    edge(f, relay, seq(ty("char *"), ty("void *"))),
    edge(relay, g, None),
    edge(g, f, seq(ty("int *"), ty("void *"))),
]
m.materialize_project_solved_param_summaries(entries, details, dep_sccs(f, relay, g), edges)
canon = canonical_by_name(entries)
assert canon[f] == "char * => void * => int * => void *", canon[f]
assert canon[relay] == "int * => void * => char * => void *", canon[relay]
for e in entries:
    assert "solvedParamSummaries" in e and "canonicalProjectParamHistories" in e

print("C6g5v import/canonical separation smoke: OK")

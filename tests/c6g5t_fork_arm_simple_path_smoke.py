#!/usr/bin/env python3
"""Smoke-test C6g5t fork/arm-aware project parameter materialization."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module_path = root / "scripts" / "build_summary_index.py"
spec = importlib.util.spec_from_file_location("c6g5t_build_summary_index", module_path)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def ty(text: str) -> dict[str, object]:
    return {"kind": "asType", "type": {"text": text, "canonical": text, "isPointer": text.endswith("*")}}


def seq(*items: dict[str, object]) -> dict[str, object]:
    out: dict[str, object] | None = None
    for item in items:
        out = module.compose_ir(out, item)
    assert out is not None
    return out


def edge(caller: str, callee: str, path: dict[str, object]) -> dict[str, object]:
    return {
        "kind": "direct-call",
        "callerNode": caller,
        "calleeNode": callee,
        "callerParamIndex": 0,
        "calleeParamIndex": 0,
        "actualPathFromCallerParam": {"ir": path},
        "guard": None,
    }


a = "relay_a::fn:(ptr:void,int:unsigned int)->void"
b = "relay_b::fn:(ptr:void,int:unsigned int)->void"
solved = {
    a: {0: {"index": 0, "ir": ty("void *"), "projectBaseIr": ty("void *"), "projectTrace": []}},
    b: {0: {"index": 0, "ir": ty("void *"), "projectBaseIr": ty("void *"), "projectTrace": []}},
}
edges = [
    edge(a, b, seq(ty("int *"), ty("void *"))),
    edge(a, b, seq(ty("char *"), ty("void *"))),
    edge(b, a, seq(ty("short *"), ty("void *"))),
]
module.solve_recursive_scc_by_simple_paths(
    solved, edges, {a, b}, {a: 0, b: 0},
    algebra=module.ProjectSummaryAlgebra.from_env(), known_param_counts={a: 2, b: 2},
)
record = solved[a][0]
rendered = module.render_ir(record["ir"])
assert "int * => void * => short * => void *" in rendered, rendered
assert "char * => void * => short * => void *" in rendered, rendered
forks = record.get("forkArmProvenance")
assert isinstance(forks, list) and len(forks) == 2, forks
assert {f["origin"] for f in forks} == {"root-actual-path"}, forks
assert all(f["forkId"] == f"{a}.$0:call-actual-fork" for f in forks), forks
assert any("int *" in f["rendered"] for f in forks), forks
assert any("char *" in f["rendered"] for f in forks), forks

# p127-style direct guarded transformer stays one bounded relation even when
# it is embedded in a larger report tree.
ptr = {"kind": "ptrarith", "type": {"text": "unsigned char *", "canonical": "unsigned char *"}}
guarded = {
    "kind": "join",
    "alternatives": [
        {"guard": {"kind": "if-then", "text": "tag"}, "history": ptr},
        {"guard": {"kind": "if-else", "text": "!(tag)"}, "history": ptr},
    ],
}
wrapped = seq(ptr, guarded)
canon = module.canonical_guarded_ptrarith_transformer(wrapped)
assert canon is not None
canon_rendered = module.render_ir(canon)
assert canon_rendered.count("ptrarith(unsigned char *)") == 2, canon_rendered
assert "[tag]" in canon_rendered and "[!(tag)]" in canon_rendered, canon_rendered

print("C6g5t fork/arm simple-path smoke: OK")

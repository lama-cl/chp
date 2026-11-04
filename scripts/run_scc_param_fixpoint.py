#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


def run(cmd: list[str], cwd: Path | None = None) -> None:
    print("[run]", " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8", errors="replace"))


def build_index(index_script: Path, summary_dir: Path, project_kind: str) -> Path:
    out = summary_dir / "project-summary-index.json"
    run([sys.executable, str(index_script), str(summary_dir), "-o", str(out), "--project-kind", project_kind])
    return out


def extract_recursive_public_keys(index: dict[str, Any]) -> set[str]:
    out: set[str] = set()
    dep_sccs = index.get("summaryDependencySCCs")
    if not isinstance(dep_sccs, dict):
        return out
    concrete = dep_sccs.get("concreteFunctionGraph")
    if not isinstance(concrete, dict):
        return out
    components = concrete.get("components")
    if not isinstance(components, list):
        return out
    for comp in components:
        if not isinstance(comp, dict):
            continue
        if not bool(comp.get("recursive")):
            continue
        members = comp.get("members")
        if not isinstance(members, list):
            continue
        for m in members:
            if isinstance(m, str) and m:
                out.add(m)
    return out


def function_key(detail: dict[str, Any]) -> str:
    ident = detail.get("identity") if isinstance(detail.get("identity"), dict) else {}
    qn = ident.get("qualifiedName") or detail.get("function") or ""
    sig = ident.get("signatureKey") or ""
    return f"{qn}::{sig}"


def function_param_fingerprint(detail: dict[str, Any]) -> str:
    params = detail.get("params")
    if not isinstance(params, list):
        params = []
    payload = []
    for p in params:
        if not isinstance(p, dict):
            continue
        # C4Q2: the SCC driver must continue iterating when project-SCC
        # pointer-use summaries change, even if the cast-history summaries are
        # already stable.  The tool emits parameter-local pointerUses in
        # functionSummaryDetails; include them in the convergence fingerprint so
        # recursive imported pointer-use facts can make another round.
        pointer_uses = p.get("pointerUses")
        if not isinstance(pointer_uses, list):
            pointer_uses = []
        payload.append({
            "index": p.get("index"),
            "name": p.get("name"),
            "role": p.get("role"),
            "behavior": p.get("behavior"),
            "summary": p.get("summary"),
            "pointerUses": pointer_uses,
        })
    return json.dumps(payload, sort_keys=True, separators=(",", ":"))


def extract_param_fingerprints(summary_dir: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for path in sorted(summary_dir.rglob("*.cast-summary.json")):
        data = load_json(path)
        details = data.get("functionSummaryDetails")
        if not isinstance(details, list):
            continue
        for d in details:
            if not isinstance(d, dict):
                continue
            key = function_key(d)
            if key:
                out[key] = function_param_fingerprint(d)
    return out


def run_tool(tool: Path, build_path: str, sources: list[str], summary_dir: Path,
             summary_index: Path | None, summary_config_id: str | None,
             extra_tool_args: list[str]) -> None:
    cmd = [str(tool)]
    if build_path:
        cmd += ["-p", build_path]
    cmd += ["--summary-dir", str(summary_dir)]
    if summary_config_id:
        cmd += ["--summary-config-id", summary_config_id]
    if summary_index is not None:
        cmd += ["--enable-summary-import", "--summary-index", str(summary_index)]
    cmd += extra_tool_args
    cmd += sources
    run(cmd)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Project-global SCC parameter-summary fixpoint driver for cast-history-tool")
    ap.add_argument("sources", nargs="+", help="Translation units to analyze")
    ap.add_argument("--tool", type=Path, required=True, help="Path to cast-history-tool")
    ap.add_argument("--index-script", type=Path, default=Path("scripts/build_summary_index.py"),
                    help="Path to build_summary_index.py")
    ap.add_argument("-p", "--build-path", default="", help="Compilation database build directory")
    ap.add_argument("--work-dir", type=Path, default=Path(".tmp-scc-fixpoint"),
                    help="Directory for iteration summaries and indices")
    ap.add_argument("--project-kind", choices=["real", "example-suite"], default="real")
    ap.add_argument("--bootstrap-index", type=Path, default=None,
                    help="Optional existing project-summary-index.json to use as iteration 0")
    ap.add_argument("--max-iters", type=int, default=8)
    ap.add_argument("--summary-config-id", default="")
    ap.add_argument("--extra-tool-arg", action="append", default=[],
                    help="Additional argument passed through to cast-history-tool")
    ap.add_argument("--keep-going-on-no-recursive-scc", action="store_true",
                    help="Continue one import round even when no recursive concrete SCCs are present")
    args = ap.parse_args()

    work = args.work_dir.resolve()
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True, exist_ok=True)

    history: list[dict[str, Any]] = []

    if args.bootstrap_index is not None:
        prev_index = args.bootstrap_index.resolve()
        prev_recursive = extract_recursive_public_keys(load_json(prev_index))
        history.append({
            "iteration": 0,
            "kind": "bootstrap-index",
            "index": str(prev_index),
            "recursiveFunctions": sorted(prev_recursive),
        })
    else:
        iter0 = work / "iter0-bootstrap"
        iter0.mkdir(parents=True, exist_ok=True)
        run_tool(args.tool, args.build_path, args.sources, iter0, None,
                 args.summary_config_id, args.extra_tool_arg)
        prev_index = build_index(args.index_script, iter0, args.project_kind)
        prev_recursive = extract_recursive_public_keys(load_json(prev_index))
        history.append({
            "iteration": 0,
            "kind": "bootstrap-run",
            "summaryDir": str(iter0),
            "index": str(prev_index),
            "recursiveFunctions": sorted(prev_recursive),
        })

    prev_fp = extract_param_fingerprints(prev_index.parent)
    converged = False
    final_index = prev_index
    final_summary_dir = prev_index.parent

    for iter_no in range(1, args.max_iters + 1):
        if not prev_recursive and not args.keep_going_on_no_recursive_scc:
            history.append({
                "iteration": iter_no,
                "kind": "stop",
                "reason": "no-recursive-concrete-scc",
            })
            converged = True
            break

        cur_dir = work / f"iter{iter_no}"
        cur_dir.mkdir(parents=True, exist_ok=True)
        run_tool(args.tool, args.build_path, args.sources, cur_dir, prev_index,
                 args.summary_config_id, args.extra_tool_arg)
        cur_index = build_index(args.index_script, cur_dir, args.project_kind)
        cur_recursive = extract_recursive_public_keys(load_json(cur_index))
        cur_fp = extract_param_fingerprints(cur_dir)

        relevant = sorted(prev_recursive | cur_recursive)
        changed = [k for k in relevant if prev_fp.get(k, "") != cur_fp.get(k, "")]

        history.append({
            "iteration": iter_no,
            "kind": "import-run",
            "summaryDir": str(cur_dir),
            "index": str(cur_index),
            "recursiveFunctions": sorted(cur_recursive),
            "changedFunctions": changed,
            "numChangedFunctions": len(changed),
        })

        final_index = cur_index
        final_summary_dir = cur_dir
        if not changed:
            converged = True
            break

        prev_index = cur_index
        prev_recursive = cur_recursive
        prev_fp = cur_fp

    report = {
        "schemaName": "cast-history-scc-param-fixpoint-report",
        "schemaVersion": 1,
        "converged": converged,
        "iterationsCompleted": len([h for h in history if h.get("kind") == "import-run"]),
        "finalIndex": str(final_index),
        "finalSummaryDir": str(final_summary_dir),
        "history": history,
    }
    report_path = work / "scc-param-fixpoint-report.json"
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print("\n[result]")
    print(f"  converged: {'yes' if converged else 'no'}")
    print(f"  final summary dir: {final_summary_dir}")
    print(f"  final index: {final_index}")
    print(f"  report: {report_path}")
    return 0 if converged else 2


if __name__ == "__main__":
    raise SystemExit(main())

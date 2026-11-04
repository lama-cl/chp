#!/usr/bin/env python3
"""Inspect/query a cast-history project summary index."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


def load_index(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
    if not isinstance(data, dict):
        raise ValueError("index top-level JSON must be object")
    return data


def entry_matches(entry: dict[str, Any], query: str) -> bool:
    q = query.lower()
    fields = [
        entry.get("function", ""),
        entry.get("name", ""),
        entry.get("qualifiedName", ""),
        entry.get("signatureKey", ""),
        entry.get("lookupKey", ""),
        entry.get("publicLookupKey", ""),
        entry.get("sourceFile", ""),
        entry.get("summaryFile", ""),
        entry.get("functionBehavior", ""),
        entry.get("status", ""),
        entry.get("resolution", ""),
    ]
    return any(q in str(f).lower() for f in fields)


def print_counts(label: str, counts: Any, indent: str = "  ") -> None:
    if not isinstance(counts, dict) or not counts:
        return
    print(f"{indent}{label}:")
    for k, v in sorted(counts.items()):
        print(f"{indent}  {k}: {v}")


def print_summary(index: dict[str, Any]) -> None:
    groups = index.get("groupCounts", {}) if isinstance(index.get("groupCounts"), dict) else {}
    print("Project summary index")
    print(f"  schema:         {index.get('schemaName','')} v{index.get('schemaVersion','')}")
    print(f"  project kind:   {index.get('projectKind','real')}")
    print(f"  summaryRoot:    {index.get('summaryRoot','')}")
    print(f"  summary files:  {index.get('numSummaryFiles', 0)}")
    print(f"  functions:      {index.get('numFunctions', 0)}")
    print(f"  dep edges:      {index.get('numSummaryDependencyEdges', 0)}")
    print(f"  branch families:{index.get('numBranchFamilies', 0)} "
          f"(arms: {index.get('numBranchFamilyTransferEdges', 0)})")
    print(f"  duplicates:     {groups.get('duplicates', len(index.get('duplicateGroups', []) or []))}")
    print(f"  ambiguities:    {groups.get('ambiguities', len(index.get('ambiguityGroups', []) or []))}")
    print(f"  problems:       {groups.get('problems', len(index.get('problemGroups', []) or []))}")
    print(f"  scoped groups:  {groups.get('scopedNameGroups', len(index.get('scopedNameGroups', []) or []))}")
    print(f"  issues:         {len(index.get('issues', []) or [])}")
    print_counts("status counts", index.get("statusCounts"))
    print_counts("resolution counts", index.get("resolutionCounts"))
    dep_graph = index.get("summaryDependencyGraph") if isinstance(index.get("summaryDependencyGraph"), dict) else {}
    print_counts("dependency edge counts", dep_graph.get("kindCounts") if isinstance(dep_graph, dict) else None)
    dep_sccs = index.get("summaryDependencySCCs") if isinstance(index.get("summaryDependencySCCs"), dict) else {}
    concrete = dep_sccs.get("concreteFunctionGraph") if isinstance(dep_sccs.get("concreteFunctionGraph"), dict) else {}
    full = dep_sccs.get("fullGraph") if isinstance(dep_sccs.get("fullGraph"), dict) else {}
    if concrete or full:
        print("  dependency SCCs:")
        if concrete:
            print(f"    concrete nontrivial/total: {concrete.get('numNontrivialSCCs', 0)} / {concrete.get('numSCCs', 0)}")
        if full:
            print(f"    full nontrivial/total:     {full.get('numNontrivialSCCs', 0)} / {full.get('numSCCs', 0)}")


def print_entry(entry: dict[str, Any], *, index_path: Path) -> None:
    sf = entry.get("summaryFile", "")
    resolved = (index_path.parent / sf).resolve() if sf else None
    print(entry.get("qualifiedName") or entry.get("function") or "<function>")
    print(f"  function:       {entry.get('function','')}")
    print(f"  signature:      {entry.get('signatureKey','')}")
    print(f"  linkage:        {entry.get('linkage','')}")
    print(f"  status:         {entry.get('status','')}")
    print(f"  resolution:     {entry.get('resolution','')}")
    print(f"  behavior:       {entry.get('functionBehavior','')}")
    print(f"  sourceFile:     {entry.get('sourceFile','')}")
    loc = entry.get("location") if isinstance(entry.get("location"), dict) else {}
    if loc:
        print(f"  location:       {loc.get('file','')}:{loc.get('line','')}:{loc.get('column','')}")
    print(f"  summaryFile:    {sf}")
    if resolved:
        print(f"  summaryPath:    {resolved}")
    print(f"  lookupKey:      {entry.get('lookupKey','')}")
    print(f"  publicKey:      {entry.get('publicLookupKey','')}")


def print_groups(index: dict[str, Any], group_name: str, *, limit: int) -> None:
    groups = index.get(group_name, [])
    if not isinstance(groups, list) or not groups:
        print(f"No {group_name}.")
        return
    functions = index.get("functions", [])
    if not isinstance(functions, list):
        functions = []
    print(f"{group_name}: {len(groups)}")
    for i, g in enumerate(groups[:limit]):
        if not isinstance(g, dict):
            continue
        print(f"\n[{i}] {g.get('kind','unknown')}  resolution={g.get('resolution','')}")
        if "lookupKey" in g:
            print(f"  lookupKey:       {g.get('lookupKey')}")
        if "publicLookupKey" in g:
            print(f"  publicLookupKey: {g.get('publicLookupKey')}")
        print(f"  count:           {g.get('count')}")
        if g.get("note"):
            print(f"  note:            {g.get('note')}")
        for idx in g.get("entries", []) or []:
            if isinstance(idx, int) and 0 <= idx < len(functions) and isinstance(functions[idx], dict):
                e = functions[idx]
                print(f"    - {e.get('qualifiedName') or e.get('function')} "
                      f"{e.get('signatureKey','')} "
                      f"[{e.get('linkage','')}; {e.get('status','')}] "
                      f"{e.get('summaryFile','')}")
    if len(groups) > limit:
        print(f"\n... {len(groups) - limit} more groups")




def edge_matches(edge: dict[str, Any], query: str) -> bool:
    q = query.lower()
    fields = [
        edge.get("kind", ""),
        edge.get("caller", ""),
        edge.get("callerQualifiedName", ""),
        edge.get("callerNode", ""),
        edge.get("callee", ""),
        edge.get("calleeQualifiedName", ""),
        edge.get("calleeNode", ""),
        edge.get("calleeExpr", ""),
        edge.get("via", ""),
        edge.get("detail", ""),
        edge.get("summaryFile", ""),
        edge.get("tuSourceFile", ""),
    ]
    return any(q in str(f).lower() for f in fields)


def print_dependency_edges(index: dict[str, Any], *, query: str | None, limit: int,
                           as_json: bool) -> None:
    edges = index.get("summaryDependencyEdges", [])
    if not isinstance(edges, list):
        print("error: index.summaryDependencyEdges must be array", file=sys.stderr)
        return
    if query:
        edges = [e for e in edges if isinstance(e, dict) and edge_matches(e, query)]
    else:
        edges = [e for e in edges if isinstance(e, dict)]
    if as_json:
        print(json.dumps(edges[:limit], indent=2, sort_keys=False))
        return
    print(f"Summary dependency edges: {len(edges)}")
    for e in edges[:limit]:
        loc = e.get("location") if isinstance(e.get("location"), dict) else {}
        where = f"{loc.get('file','')}:{loc.get('line','')}:{loc.get('column','')}" if loc else ""
        print(f"  {e.get('callerNode') or e.get('caller')} -> {e.get('calleeNode') or e.get('callee')} [{e.get('kind','unknown')}]")
        if e.get("via") or e.get("calleeExpr"):
            print(f"    via: {e.get('via','')}  expr: {e.get('calleeExpr','')}")
        if e.get("guard"):
            g = e.get("guard") if isinstance(e.get("guard"), dict) else {}
            print(f"    guard: {g.get('text','')}")
        if where:
            print(f"    at: {where}")
    if len(edges) > limit:
        print(f"... {len(edges) - limit} more edges")




def print_branch_families(index: dict[str, Any], *, query: str | None,
                          limit: int, as_json: bool) -> None:
    data = index.get("branchFamilyTransferMetadata", {})
    if not isinstance(data, dict):
        print("No branchFamilyTransferMetadata in index.")
        return
    families = data.get("families", [])
    if not isinstance(families, list):
        print("error: branchFamilyTransferMetadata.families must be array", file=sys.stderr)
        return
    families = [x for x in families if isinstance(x, dict)]
    if query:
        q = query.lower()
        def matches(f: dict[str, Any]) -> bool:
            fields = [f.get("id", ""), f.get("kind", ""), f.get("ownerNode", ""),
                      f.get("origin", ""), f.get("recursiveClosureKey", "")]
            for arm in f.get("arms", []) or []:
                if isinstance(arm, dict):
                    fields.extend([arm.get("id", ""), arm.get("actualPath", ""),
                                   arm.get("guardText", ""),
                                   arm.get("callerNode", ""), arm.get("calleeNode", ""),
                                   arm.get("tuSourceFile", "")])
            return any(q in str(v).lower() for v in fields)
        families = [f for f in families if matches(f)]
    if as_json:
        print(json.dumps(families[:limit], indent=2, sort_keys=False))
        return
    counts = data.get("counts", {}) if isinstance(data.get("counts"), dict) else {}
    print(f"Branch-family transfer metadata: {len(families)} family/families "
          f"(index arms: {counts.get('arms', 0)})")
    contract = data.get("rehydrationContract")
    if contract:
        print(f"  import rehydration: {contract}")
    for family in families[:limit]:
        print(f"  family={family.get('id','')} [kind={family.get('kind','unknown')}]")
        print(f"    owner={family.get('ownerNode','')}.$"
              f"{family.get('ownerParamIndex','')}")
        if family.get("origin"):
            print(f"    origin={family.get('origin')}")
        if family.get("recursiveClosureKey"):
            print(f"    recursive-closure-key={family.get('recursiveClosureKey')}")
        if family.get("consumedByTaggedRecursiveSolver"):
            print("    recursive-closure-consumed-by=C2 tagged recursive summary materialization")
        for arm in family.get("arms", []) or []:
            if not isinstance(arm, dict):
                continue
            print(f"    arm={arm.get('id','')} [kind={arm.get('kind','unknown')}]"
                  f" : {arm.get('actualPath','')}")
            print(f"      transfer={arm.get('callerNode','')}.$"
                  f"{arm.get('callerParamIndex','')} -> {arm.get('calleeNode','')}.$"
                  f"{arm.get('calleeParamIndex','')}")
            if arm.get("guardText"):
                print(f"      arm-guard={arm.get('guardText','')}")
            loc = arm.get("callSite") if isinstance(arm.get("callSite"), dict) else {}
            if loc:
                print(f"      @ {loc.get('file','')}:{loc.get('line','')}:{loc.get('column','')}")
            consumed = ("; recursive-closure-key-consumed-by=C2-tagged-solver"
                        if family.get("consumedByTaggedRecursiveSolver") else "")
            print("      transport=tu-summary -> project-index "
                  "(SummaryIndex companion-rehydration available)" + consumed)
    if len(families) > limit:
        print(f"... {len(families) - limit} more family/families")


def print_dependency_sccs(index: dict[str, Any], *, concrete: bool, limit: int,
                          as_json: bool) -> None:
    sccs = index.get("summaryDependencySCCs")
    if not isinstance(sccs, dict):
        print("No summaryDependencySCCs in index.")
        return
    graph_name = "concreteFunctionGraph" if concrete else "fullGraph"
    graph = sccs.get(graph_name)
    if not isinstance(graph, dict):
        print(f"No {graph_name} SCC report in index.")
        return
    if as_json:
        print(json.dumps(graph, indent=2, sort_keys=False))
        return
    print(f"Summary dependency SCCs ({graph.get('graphKind', graph_name)}):")
    print(f"  nodes:            {graph.get('numNodes', 0)}")
    print(f"  unique edges:     {graph.get('numUniqueEdges', 0)}")
    print(f"  SCCs:             {graph.get('numSCCs', 0)}")
    print(f"  nontrivial SCCs:  {graph.get('numNontrivialSCCs', 0)}")
    comps = graph.get("nontrivialComponents")
    label = "Nontrivial components"
    if not isinstance(comps, list) or not comps:
        comps = graph.get("components") if isinstance(graph.get("components"), list) else []
        label = "Components"
    if not comps:
        return
    print(f"  {label}:")
    for comp in comps[:limit]:
        if not isinstance(comp, dict):
            continue
        print(f"    SCC #{comp.get('sccId', '')}: size={comp.get('size', 0)} recursive={comp.get('recursive', False)}")
        for member in comp.get("members", []) or []:
            print(f"      - {member}")
    if len(comps) > limit:
        print(f"  ... {len(comps) - limit} more components")

def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Inspect cast-history project summary index")
    ap.add_argument("index", type=Path, help="project-summary-index.json")
    ap.add_argument("query", nargs="?", default=None, help="Search query, e.g. function name or status")
    ap.add_argument("--ambiguous", action="store_true", help="Show ambiguityGroups")
    ap.add_argument("--duplicates", action="store_true", help="Show duplicateGroups")
    ap.add_argument("--problems", action="store_true", help="Show hard problemGroups")
    ap.add_argument("--scoped", action="store_true", help="Show source-scoped same-name groups")
    ap.add_argument("--deps", action="store_true", help="Show summary dependency edges")
    ap.add_argument("--branch-families", action="store_true",
                    help="Show C6g5w1 diagnostics-only branch-family transfer metadata")
    ap.add_argument("--sccs", action="store_true", help="Show summary dependency SCCs")
    ap.add_argument("--full-sccs", action="store_true", help="With --sccs, include symbolic callback nodes such as $0")
    ap.add_argument("--json", action="store_true", help="Print matching entries as JSON")
    ap.add_argument("--limit", type=int, default=50, help="Maximum matches/groups to print")
    args = ap.parse_args(argv)

    index = load_index(args.index)
    if args.ambiguous:
        print_groups(index, "ambiguityGroups", limit=args.limit)
        return 0
    if args.duplicates:
        print_groups(index, "duplicateGroups", limit=args.limit)
        return 0
    if args.problems:
        print_groups(index, "problemGroups", limit=args.limit)
        return 0
    if args.scoped:
        print_groups(index, "scopedNameGroups", limit=args.limit)
        return 0
    if args.branch_families:
        print_branch_families(index, query=args.query, limit=args.limit,
                              as_json=args.json)
        return 0
    if args.deps:
        print_dependency_edges(index, query=args.query, limit=args.limit,
                               as_json=args.json)
        return 0
    if args.sccs:
        print_dependency_sccs(index, concrete=not args.full_sccs,
                              limit=args.limit, as_json=args.json)
        return 0

    functions = index.get("functions", [])
    if not isinstance(functions, list):
        print("error: index.functions must be array", file=sys.stderr)
        return 1
    if args.query is None:
        print_summary(index)
        return 0
    matches = [e for e in functions if isinstance(e, dict) and entry_matches(e, args.query)]
    if args.json:
        print(json.dumps(matches[:args.limit], indent=2, sort_keys=False))
        return 0
    print(f"Matches for {args.query!r}: {len(matches)}")
    for i, e in enumerate(matches[:args.limit]):
        if i:
            print()
        print_entry(e, index_path=args.index)
    if len(matches) > args.limit:
        print(f"\n... {len(matches) - args.limit} more matches")
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

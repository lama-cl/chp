#!/usr/bin/env python3
"""Pointer-coverage ledger for cast-history paper-facts sidecars.

Every pointer subject in every ``*.cast-paper-facts.json`` under ``--root`` is
put in exactly one bucket:

  resolved              concrete interpretation evidence reached the subject
  blocked:external      lost at a call into code with no body and no model
  blocked:callback-*    lost at an indirect call (unresolved / field / formal)
  blocked:<new reason>  reserved for reasons added by later phases
  opaque-no-evidence    only transported; nothing blocked it, nothing typed it

The ledger is the measuring stick of docs/pointer-coverage-plan.md: phases
must move subjects out of the ``blocked:*`` buckets without growing
``opaque-no-evidence``.  It reads only the existing paper-facts schema, so it
works on the unmodified tool.

Usage:
  coverage_ledger.py --root OUT_DIR [--json ledger.json]
  coverage_ledger.py --root OUT_DIR --compare OLD_LEDGER.json   # ratchet
"""
import argparse
import collections
import json
import pathlib
import sys

POINTER_KINDS = ("void-data-pointer", "nonvoid-data-pointer", "function-pointer")


def concrete_types(subject):
    types = set()
    for inst in subject.get("historyInstances") or []:
        types.update((inst.get("metrics") or {}).get("nonVoidPointerTypes") or [])
    types.update((subject.get("refinementFacts") or {}).get("activeConcreteTypes") or [])
    return {" ".join(t.replace("*", " * ").split()) for t in types}


def bucket_of(subject):
    """Single bucket for one paper-facts subject row."""
    if concrete_types(subject):
        return "resolved"
    # Phase 0 adds an explicit `escape.reason`; prefer it once it exists.
    reason = (subject.get("escape") or {}).get("reason")
    if reason:
        return "blocked:" + reason
    state = (subject.get("callbackMediation") or {}).get("state", "None")
    if state == "UnresolvedCallback":
        return "blocked:callback-unresolved"
    if state == "FieldCallback":
        return "blocked:callback-field"
    if state == "FormalCallbackParameter":
        return "blocked:callback-formal"
    ops = subject.get("operations") or {}
    if ops.get("externalEscape") or subject.get("primaryCategory") == "ExternalUnknown":
        return "blocked:external"
    return "opaque-no-evidence"


def build_ledger(root):
    seen = {}
    files = sorted(pathlib.Path(root).rglob("*.cast-paper-facts.json"))
    for path in files:
        try:
            doc = json.loads(path.read_text())
        except (OSError, ValueError) as err:
            print(f"warning: skipping {path}: {err}", file=sys.stderr)
            continue
        for s in doc.get("subjects") or []:
            kind = (s.get("declaredType") or {}).get("outerPointerKind")
            if kind not in POINTER_KINDS:
                continue
            loc = s.get("location") or {}
            key = (loc.get("file"), loc.get("line"), loc.get("column"), s.get("entity"))
            bucket = bucket_of(s)
            # A header-declared subject can appear in many TUs: keep the best.
            if key not in seen or (bucket == "resolved" and seen[key][1] != "resolved"):
                seen[key] = (kind, bucket)
    by_kind = {k: collections.Counter() for k in POINTER_KINDS}
    for kind, bucket in seen.values():
        by_kind[kind][bucket] += 1
    out = {"schemaName": "cast-history-pointer-coverage-ledger", "schemaVersion": 1,
           "paperFactsFiles": len(files), "kinds": {}}
    for kind, counts in by_kind.items():
        total = sum(counts.values())
        blocked = sum(n for b, n in counts.items() if b.startswith("blocked:"))
        out["kinds"][kind] = {
            "total": total,
            "resolved": counts.get("resolved", 0),
            "blocked": blocked,
            "opaqueNoEvidence": counts.get("opaque-no-evidence", 0),
            "resolvedShare": round(counts.get("resolved", 0) / total, 4) if total else None,
            "blockedShare": round(blocked / total, 4) if total else None,
            "buckets": dict(sorted(counts.items())),
        }
    return out


def render(ledger):
    lines = [f"paper-facts files: {ledger['paperFactsFiles']}"]
    for kind, row in ledger["kinds"].items():
        lines.append(f"{kind}: total={row['total']} resolved={row['resolved']} "
                     f"blocked={row['blocked']} opaque={row['opaqueNoEvidence']}")
        for bucket, n in row["buckets"].items():
            lines.append(f"    {bucket:32s} {n}")
    return "\n".join(lines)


def compare(new, old):
    """Ratchet: resolved may not drop, blocked may not grow, per pointer kind."""
    problems = []
    for kind, row in new["kinds"].items():
        prev = (old.get("kinds") or {}).get(kind)
        if not prev:
            continue
        if row["resolved"] < prev["resolved"]:
            problems.append(f"{kind}: resolved dropped {prev['resolved']} -> {row['resolved']}")
        if row["blocked"] > prev["blocked"]:
            problems.append(f"{kind}: blocked grew {prev['blocked']} -> {row['blocked']}")
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", required=True, help="directory containing *.cast-paper-facts.json")
    ap.add_argument("--json", help="write the ledger JSON here")
    ap.add_argument("--compare", help="older ledger JSON; exit 1 if coverage regressed")
    args = ap.parse_args()
    ledger = build_ledger(args.root)
    print(render(ledger))
    if args.json:
        pathlib.Path(args.json).write_text(json.dumps(ledger, indent=2) + "\n")
    if args.compare:
        problems = compare(ledger, json.loads(pathlib.Path(args.compare).read_text()))
        for p in problems:
            print("REGRESSION:", p)
        return 1 if problems else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())

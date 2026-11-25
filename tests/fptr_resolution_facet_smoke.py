#!/usr/bin/env python3
"""Smoke-test the schema-v10 indirectCallResolution facet (validator + aggregator)."""
from __future__ import annotations

import copy
import csv
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
validator = root / "scripts" / "validate_paper_facts.py"
aggregator = root / "scripts" / "aggregate_paper_facts.py"
failures: list[str] = []


def check(cond: bool, message: str) -> None:
    if not cond:
        failures.append(message)


def site(state="exact", layer="L0", closure=None, mismatch=False, composition="full"):
    if state == "parametric" and closure is None:
        closure = "closed"
    return {"callee": "cb", "slot": None, "state": state, "layer": layer,
            "targets": ["f"], "guarded": False, "indirectHistoryComposition": composition,
            "parametricClosure": closure, "signatureMismatch": mismatch,
            "span": {"file": "a.c", "line": 1, "column": 1}}


def facet(state, layer, sites, mismatch=False):
    return {"state": state, "layer": layer, "signatureMismatch": mismatch, "sites": sites}


def subject(sid, category, res, callback="None"):
    s = {"subjectId": sid, "rootSubjectId": sid, "entity": sid, "subjectKind": "local",
         "declaredType": {"outerPointerKind": "unknown", "outerVoidPointer": False},
         "primaryCategory": category, "historyInstances": [], "history": {}, "operations": {},
         "refinementFacts": {},
         "callbackMediation": {"state": callback, "flags": {
             "callback": False, "resolvedTarget": False, "fieldCarrier": False,
             "guardedSelection": False}}}
    if res is not None:
        s["indirectCallResolution"] = res
    return s


def document(version, subjects):
    coverage = {k: "m" for k in (
        "arrayIndexDirectionModel", "declaredOuterPointerKindModel", "callbackMediationModel",
        "fieldStorageClassificationModel", "declaredPointerInventoryModel",
        "declaredPointerInventoryScope", "nestedCallbackSlotRelationModel",
        "voidPointerUsageFingerprintModel", "observedNoFinalHistoryPatternModel",
        "allocationOriginDerivedAliasModel")}
    for k in ("subjectRows", "subjectsWithFinalHistory", "callbackUseRecords",
              "declaredPointerInventoryRows", "declaredEligibleDataPointerRows",
              "declaredEligibleVoidPointerRows", "declaredEligibleVoidPointerSubjectFacts",
              "declaredEligibleVoidPointerFinalHistory", "declaredEligibleVoidPointerClassified",
              "declaredEligibleVoidPointerNoObservedFacts", "finalHistoryWithoutSubjectRow",
              "nestedCallbackSlotRelations", "voidPointerUsageFingerprints",
              "allocationOriginDerivedAliasUses", "declaredDirectVoidPointerInventoryRows",
              "declaredExcludedUnavailableApiVoidPointerRows", "eligibleNestedCallbackSlotRelations",
              "excludedUnavailableApiNestedCallbackSlots"):
        coverage[k] = 0
    coverage["declaredPointerInventoryScope"] = "project-root-v1"
    coverage["declaredPointerInventoryScopeComplete"] = True
    return {"schemaName": "cast-history-tu-paper-facts", "schemaVersion": version,
            "tu": {"sourceFile": "a.c"}, "coverage": coverage, "subjects": subjects,
            "callbackUseRecords": [], "declaredPointerInventory": [],
            "nestedCallbackSlotRelations": [], "voidPointerUsageFingerprints": [],
            "allocationOriginDerivedAliasUses": []}


def run_validator(td: Path, name: str, doc):
    path = td / f"{name}.cast-paper-facts.json"
    path.write_text(json.dumps(doc), encoding="utf-8")
    return subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)


OPAQUE = "OpaqueTransportOrViewOnly"
with tempfile.TemporaryDirectory(prefix="fptr-facet-") as tmp:
    td = Path(tmp)
    none_res = facet("none", "-", [])
    exact_res = facet("exact", "L0", [site()])

    def expect(name, version, res, ok, needle="", callback="None"):
        r = run_validator(td, name, document(version, [subject("s", OPAQUE, res, callback)]))
        check((r.returncode == 0) == ok, f"{name}: expected {'pass' if ok else 'fail'}, rc={r.returncode}: {r.stderr}")
        if needle:
            check(needle in r.stderr, f"{name}: stderr lacks {needle!r}: {r.stderr}")
        return r

    # Version gating.
    expect("v10_valid_none", 10, none_res, True)
    expect("v10_valid_exact", 10, exact_res, True)
    expect("v10_missing_key", 10, None, False, "indirectCallResolution must be an object")
    expect("v9_without_key", 9, None, True)
    # Rule 2.
    expect("none_with_sites", 10, facet("none", "-", [site()]), False, "none iff sites")
    expect("sites_but_none_state", 10, facet("none", "L0", [site()]), False, "none iff sites")
    expect("empty_sites_not_none", 10, facet("exact", "L0", []), False, "none iff sites")
    expect("none_wrong_layer", 10, facet("none", "L0", []), False, "layer must be '-'")
    # Rule 3.
    two = [site("exact"), site("set", "L1")]
    expect("weakest_ok", 10, facet("set", "L1", two), True)
    expect("weakest_bad", 10, facet("exact", "L1", two), False, "weakest site state")
    tie = [site("exact"), site("parametric")]
    expect("tie_parametric", 10, facet("parametric", "L0", tie), True)
    expect("tie_exact_bad", 10, facet("exact", "L0", tie), False, "weakest site state")
    # Rule 4.
    layers = [site(layer="L1"), site("set", "L2"), site("set", "LIB")]
    expect("layer_ok", 10, facet("set", "L2", layers), True)
    expect("layer_bad", 10, facet("set", "LIB", layers), False, "highest site layer")
    # Rule 5.
    mism = [site("signature", "L0", mismatch=True), site()]
    expect("mismatch_ok", 10, facet("signature", "L0", mism, True), True)
    expect("mismatch_bad", 10, facet("signature", "L0", mism, False), False, "any(site.signatureMismatch)")
    # parametricClosure iff parametric.
    bad_closure = site("exact")
    bad_closure["parametricClosure"] = "open"
    expect("closure_on_exact", 10, facet("exact", "L0", [bad_closure]), False, "parametricClosure")
    nullclosure = site("parametric")
    nullclosure["parametricClosure"] = None
    expect("closure_missing", 10, facet("parametric", "L0", [nullclosure]), False, "parametricClosure")
    # Rule 6: warning only.
    nocomp = [site(composition="none")]
    r = expect("warn_resolved", 10, facet("exact", "L0", nocomp), True, "WARNING", "ResolvedCallback")
    r = expect("warn_guarded", 10, facet("exact", "L0", nocomp), True, "WARNING", "GuardedResolvedFieldCallback")
    r = expect("no_warn_other", 10, facet("exact", "L0", nocomp), True, "", "UnresolvedCallback")
    check("WARNING" not in r.stderr, "UnresolvedCallback must not warn")

    # Aggregator cross-tab.
    agg_root = td / "agg"
    (agg_root / "p").mkdir(parents=True)
    subs = [
        subject("a", OPAQUE, none_res),
        subject("b", OPAQUE, facet("parametric", "L0", [site("parametric", closure="closed")])),
        subject("c", OPAQUE, facet("parametric", "L0",
                                   [site("parametric", closure="closed"), site("parametric", closure="open")])),
        subject("d", "ExternalUnknown", facet("unresolved", "L2", [site("unresolved", "L2")])),
    ]
    (agg_root / "p" / "new.cast-paper-facts.json").write_text(json.dumps(document(10, subs)), encoding="utf-8")
    (agg_root / "p" / "old.cast-paper-facts.json").write_text(
        json.dumps(document(9, [subject("e", OPAQUE, None)])), encoding="utf-8")
    out = td / "out"
    r = subprocess.run([sys.executable, str(aggregator), "--root", str(agg_root), "--out-dir", str(out),
                        "--project-name", "p"], capture_output=True, text=True)
    check(r.returncode == 0, f"aggregator failed: {r.stderr}")
    tab = out / "category_by_indirect_resolution.csv"
    if tab.exists():
        rows = {row["category"]: row for row in csv.DictReader(tab.open(encoding="utf-8"))}
        o, e = rows[OPAQUE], rows["ExternalUnknown"]
        check(o["none"] == "1", f"opaque none: {o}")
        check(o["parametric(closed)"] == "1", f"opaque parametric(closed): {o}")
        check(o["parametric(open)"] == "1", f"opaque parametric(open): {o}")
        check(o["absent"] == "1", f"opaque absent: {o}")
        check(o["total"] == "4", f"opaque total: {o}")
        check(e["unresolved"] == "1" and e["total"] == "1", f"external unknown: {e}")
    else:
        failures.append("category_by_indirect_resolution.csv not written")

if failures:
    for f in failures:
        print(f"FAIL: {f}", file=sys.stderr)
    sys.exit(1)
print("fptr resolution facet smoke: ok")

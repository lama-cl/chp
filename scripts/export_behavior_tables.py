#!/usr/bin/env python3
"""
export_behavior_tables.py

C4R3: Export normalized, paper-ready tables from cast-history JSON summaries.

This script is intentionally a post-processing layer. It does not run the analyzer
and does not change propagation or classification semantics. It reads verbose C6g5f4 report sidecars (*.cast-report.json; legacy
*.cast-summary.json remains accepted) and writes stable row-oriented CSV/JSONL tables for
experiments, configuration comparison, and commit-evolution studies.

Default outputs:
  - cast_history_rows.csv/.jsonl
  - pointer_operation_rows.csv/.jsonl
  - parameter_behavior_rows.csv/.jsonl
  - function_behavior_rows.csv/.jsonl
  - manifest.json

Example:
  scripts/export_behavior_tables.py \
      --root test-output/multitu \
      --out-dir test-output/behavior-tables
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

VERSION = "v3-c6g5f4-report-export"

TABLE_NAMES = (
    "cast_history_rows",
    "pointer_operation_rows",
    "parameter_behavior_rows",
    "function_behavior_rows",
)

CAST_HISTORY_FIELDS = [
    "project",
    "project_dir",
    "summary_file",
    "source_file",
    "configuration_id",
    "compile_command_hash",
    "history_kind",
    "entity",
    "function",
    "entity_name",
    "history",
    "history_shape",
    "history_risk",
    "behavior",
    "wild_like",
    "paper_surface_kind",
    "paper_surface_reasons",
    "paper_semantic_family",
    "paper_semantic_risk_tier",
    "paper_interpretation",
    "callback_mediation_state",
    "callback_mediation_callback",
    "callback_mediation_resolved_target",
    "callback_mediation_field_carrier",
    "callback_mediation_guarded_selection",
    "semantic_mediation_kind",
    "semantic_mediation_family",
]

POINTER_OPERATION_FIELDS = [
    "project",
    "project_dir",
    "summary_file",
    "source_file",
    "configuration_id",
    "compile_command_hash",
    "entity",
    "function",
    "entity_name",
    "guard",
    "kind",
    "pointer_type",
    "access_type",
    "attach_path",
    "detail",
    "span_file",
    "span_line",
    "span_column",
    "evidence_source_kind",
    "evidence_sources",
]

PARAMETER_BEHAVIOR_FIELDS = [
    "project",
    "project_dir",
    "summary_file",
    "source_file",
    "configuration_id",
    "compile_command_hash",
    "function",
    "param_index",
    "param_name",
    "declared_type",
    "param_role",
    "summary",
    "view",
    "history_shape",
    "history_risk",
    "behavior",
    "wild_like",
    "deref_types",
    "write_deref_types",
    "num_deref_types",
    "num_write_deref_types",
    "has_void_interface",
    "has_cast_like_reinterpretation",
    "has_ptr_arith",
    "has_byte_view",
    "has_byte_view_write",
    "has_concrete_deref",
    "has_concrete_write",
    "has_multi_type_deref",
    "has_multi_type_write",
    "evidence_source_kind",
    "evidence_sources",
    "evidence_flags",
    "operation_kinds",
    "paper_surface_kind",
    "paper_surface_reasons",
    "paper_semantic_family",
    "paper_semantic_risk_tier",
    "paper_interpretation",
    "callback_mediation_state",
    "callback_mediation_callback",
    "callback_mediation_resolved_target",
    "callback_mediation_field_carrier",
    "callback_mediation_guarded_selection",
    "semantic_mediation_kind",
    "semantic_mediation_family",
]

FUNCTION_BEHAVIOR_FIELDS = [
    "project",
    "project_dir",
    "summary_file",
    "source_file",
    "configuration_id",
    "compile_command_hash",
    "function",
    "qualified_name",
    "signature_key",
    "linkage",
    "definition_file",
    "definition_line",
    "definition_column",
    "function_class",
    "classification_evidence_source_kind",
    "classification_evidence_sources",
    "classification_evidence_flags",
    "classification_operation_kinds",
    "classification_deref_sources",
    "classification_write_sources",
    "evidence_entities",
    "evidence_behaviors",
    "evidence_history_shapes",
    "evidence_history_risks",
    "evidence_deref_types",
    "evidence_write_deref_types",
    "evidence_callback_mediation_states",
    "evidence_field_callback_provider",
    "evidence_guarded_field_callback_selection",
    "evidence_resolved_callback_mediation",
]

FIELDS_BY_TABLE = {
    "cast_history_rows": CAST_HISTORY_FIELDS,
    "pointer_operation_rows": POINTER_OPERATION_FIELDS,
    "parameter_behavior_rows": PARAMETER_BEHAVIOR_FIELDS,
    "function_behavior_rows": FUNCTION_BEHAVIOR_FIELDS,
}


def _as_bool_text(value: Any) -> str:
    if value is True:
        return "yes"
    if value is False:
        return "no"
    if value is None:
        return ""
    return str(value)


def _join(values: Any, sep: str = ";") -> str:
    if values is None:
        return ""
    if isinstance(values, (str, int, float, bool)):
        return _as_bool_text(values) if isinstance(values, bool) else str(values)
    if isinstance(values, Mapping):
        parts = []
        for k in sorted(values):
            v = values[k]
            if isinstance(v, bool):
                v = _as_bool_text(v)
            parts.append(f"{k}={v}")
        return sep.join(parts)
    try:
        return sep.join(str(v) for v in values if v is not None and str(v) != "")
    except TypeError:
        return str(values)


def _nested(obj: Mapping[str, Any], path: Sequence[str], default: Any = "") -> Any:
    cur: Any = obj
    for key in path:
        if not isinstance(cur, Mapping) or key not in cur:
            return default
        cur = cur[key]
    return cur


def _rendered(obj: Any) -> str:
    if obj is None:
        return ""
    if isinstance(obj, str):
        return obj
    if isinstance(obj, Mapping):
        if "rendered" in obj:
            return str(obj.get("rendered") or "")
        if "summary" in obj:
            return _rendered(obj.get("summary"))
    return ""


def _span_value(span: Any, key: str) -> str:
    if isinstance(span, Mapping):
        v = span.get(key)
        return "" if v is None else str(v)
    return ""


def _entity_function(entity: str) -> str:
    if "::" in entity:
        return entity.split("::", 1)[0]
    return ""


def _entity_name(entity: str) -> str:
    if "::" in entity:
        return entity.split("::", 1)[1]
    return entity


def _project_dir_for_summary(summary: Path) -> Path:
    if summary.parent.name in {"summaries", "reports"}:
        return summary.parent.parent
    return summary.parent


def _rel(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def _common(summary: Path, root: Path, data: Mapping[str, Any]) -> Dict[str, str]:
    project_dir = _project_dir_for_summary(summary)
    tu = data.get("tu") if isinstance(data.get("tu"), Mapping) else {}
    return {
        "project": project_dir.name,
        "project_dir": _rel(project_dir, root),
        "summary_file": _rel(summary, root),
        "source_file": str(tu.get("sourceFile") or data.get("sourceFile") or summary.name.replace(".cast-report.json", "").replace(".cast-summary.json", "")),
        "configuration_id": str(tu.get("configurationId") or ""),
        "compile_command_hash": str(tu.get("compileCommandHash") or ""),
    }


def _paper_fields(obj: Mapping[str, Any]) -> Dict[str, str]:
    pw = obj.get("paperWildness")
    if not isinstance(pw, Mapping):
        return {
            "paper_surface_kind": "",
            "paper_surface_reasons": "",
            "paper_semantic_family": "",
            "paper_semantic_risk_tier": "",
            "paper_interpretation": "",
        }
    return {
        "paper_surface_kind": str(pw.get("surfaceKind") or ""),
        "paper_surface_reasons": _join(pw.get("surfaceReasons") or []),
        "paper_semantic_family": str(pw.get("semanticFamily") or ""),
        "paper_semantic_risk_tier": str(pw.get("semanticRiskTier") or ""),
        "paper_interpretation": str(pw.get("interpretation") or ""),
    }


def _callback_mediation_fields(obj: Mapping[str, Any]) -> Dict[str, str]:
    mediation = obj.get("callbackMediation")
    if not isinstance(mediation, Mapping):
        mediation = {}
    flags = mediation.get("flags") if isinstance(mediation.get("flags"), Mapping) else {}
    semantic = obj.get("semanticMediation")
    if not isinstance(semantic, Mapping):
        semantic = {}
    return {
        "callback_mediation_state": str(mediation.get("state") or semantic.get("callbackMediationState") or ""),
        "callback_mediation_callback": _as_bool_text(flags.get("callback")) if "callback" in flags else "",
        "callback_mediation_resolved_target": _as_bool_text(flags.get("resolvedTarget")) if "resolvedTarget" in flags else "",
        "callback_mediation_field_carrier": _as_bool_text(flags.get("fieldCarrier")) if "fieldCarrier" in flags else "",
        "callback_mediation_guarded_selection": _as_bool_text(flags.get("guardedSelection")) if "guardedSelection" in flags else "",
        "semantic_mediation_kind": str(semantic.get("kind") or ""),
        "semantic_mediation_family": str(semantic.get("semanticFamily") or ""),
    }


def _evidence_source_kind(evidence: Any) -> str:
    if isinstance(evidence, Mapping):
        if evidence.get("sourceKind"):
            return str(evidence.get("sourceKind"))
        if evidence.get("summarySourceKind"):
            return str(evidence.get("summarySourceKind"))
    return ""


def _evidence_sources(evidence: Any) -> str:
    if isinstance(evidence, Mapping):
        return _join(evidence.get("sources") or evidence.get("classificationEvidenceSources") or [])
    return ""


def _operation_kinds_from_evidence(evidence: Any) -> str:
    if not isinstance(evidence, Mapping):
        return ""
    ops = evidence.get("operations")
    if isinstance(ops, list):
        return _join(sorted({str(op.get("kind")) for op in ops if isinstance(op, Mapping) and op.get("kind")}))
    return _join(_nested(evidence, ["operations", "kinds"], []))


def _wild_pointer_metrics(obj: Mapping[str, Any]) -> Mapping[str, Any]:
    w = obj.get("wildPointerMetrics")
    return w if isinstance(w, Mapping) else {}


def _metrics(obj: Mapping[str, Any]) -> Mapping[str, Any]:
    m = obj.get("metrics")
    return m if isinstance(m, Mapping) else {}


def _index_pointer_behavior(data: Mapping[str, Any]) -> Dict[str, Mapping[str, Any]]:
    out: Dict[str, Mapping[str, Any]] = {}
    for item in data.get("pointerBehaviorMetrics", []) or []:
        if isinstance(item, Mapping) and item.get("entity"):
            out[str(item["entity"])] = item
    return out


def _index_function_behavior(data: Mapping[str, Any]) -> Dict[str, Mapping[str, Any]]:
    out: Dict[str, Mapping[str, Any]] = {}
    for item in data.get("functionBehaviorClassifications", []) or []:
        if isinstance(item, Mapping) and item.get("function"):
            out[str(item["function"])] = item
    return out


def export_cast_history_rows(summary: Path, root: Path, data: Mapping[str, Any]) -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    common = _common(summary, root, data)
    behavior_by_entity = _index_pointer_behavior(data)
    for history_kind, key in (("direct", "directCastHistories"), ("storage", "storageCastHistories")):
        for item in data.get(key, []) or []:
            if not isinstance(item, Mapping):
                continue
            entity = str(item.get("entity") or "")
            behavior = behavior_by_entity.get(entity, {})
            row = dict(common)
            row.update({
                "history_kind": history_kind,
                "entity": entity,
                "function": _entity_function(entity),
                "entity_name": _entity_name(entity),
                "history": str(item.get("history") or _rendered(item.get("summary")) or ""),
                "history_shape": str(behavior.get("historyShape") or ""),
                "history_risk": str(behavior.get("historyRisk") or ""),
                "behavior": str(behavior.get("behavior") or ""),
                "wild_like": _as_bool_text(behavior.get("wildLike")) if "wildLike" in behavior else "",
            })
            row.update(_paper_fields(behavior))
            row.update(_callback_mediation_fields(behavior))
            rows.append(row)
    return rows


def export_pointer_operation_rows(summary: Path, root: Path, data: Mapping[str, Any]) -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    common = _common(summary, root, data)
    behavior_by_entity = _index_pointer_behavior(data)
    for item in data.get("pointerUseSummaries", []) or []:
        if not isinstance(item, Mapping):
            continue
        entity = str(item.get("entity") or "")
        behavior = behavior_by_entity.get(entity, {})
        evidence = behavior.get("behaviorEvidence") if isinstance(behavior, Mapping) else {}
        for use in item.get("uses", []) or []:
            if not isinstance(use, Mapping):
                continue
            span = use.get("span") if isinstance(use.get("span"), Mapping) else {}
            row = dict(common)
            row.update({
                "entity": entity,
                "function": _entity_function(entity),
                "entity_name": _entity_name(entity),
                "guard": str(use.get("guard") or "always"),
                "kind": str(use.get("kind") or ""),
                "pointer_type": str(use.get("pointerType") or ""),
                "access_type": str(use.get("accessType") or ""),
                "attach_path": _rendered(use.get("attachPath")),
                "detail": str(use.get("detail") or ""),
                "span_file": _span_value(span, "file"),
                "span_line": _span_value(span, "line"),
                "span_column": _span_value(span, "column"),
                "evidence_source_kind": _evidence_source_kind(evidence),
                "evidence_sources": _evidence_sources(evidence),
            })
            rows.append(row)
    return rows


def export_parameter_behavior_rows(summary: Path, root: Path, data: Mapping[str, Any]) -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    common = _common(summary, root, data)
    for item in data.get("functionParameterBehaviorMetrics", []) or []:
        if not isinstance(item, Mapping):
            continue
        w = _wild_pointer_metrics(item)
        evidence = item.get("behaviorEvidence") if isinstance(item.get("behaviorEvidence"), Mapping) else {}
        row = dict(common)
        row.update({
            "function": str(item.get("function") or ""),
            "param_index": str(item.get("paramIndex") if item.get("paramIndex") is not None else item.get("index", "")),
            "param_name": str(item.get("paramName") or item.get("name") or ""),
            "declared_type": str(item.get("declaredType") or ""),
            "param_role": str(item.get("paramRole") or item.get("role") or ""),
            "summary": _rendered(item.get("summary")),
            "view": str(item.get("view") or ""),
            "history_shape": str(item.get("historyShape") or ""),
            "history_risk": str(item.get("historyRisk") or ""),
            "behavior": str(item.get("behavior") or ""),
            "wild_like": _as_bool_text(item.get("wildLike")) if "wildLike" in item else "",
            "deref_types": _join(w.get("derefTypes") or []),
            "write_deref_types": _join(w.get("writeDerefTypes") or []),
            "num_deref_types": str(w.get("numDerefTypes") if w.get("numDerefTypes") is not None else ""),
            "num_write_deref_types": str(w.get("numWriteDerefTypes") if w.get("numWriteDerefTypes") is not None else ""),
            "has_void_interface": _as_bool_text(w.get("hasVoidInterface")) if "hasVoidInterface" in w else "",
            "has_cast_like_reinterpretation": _as_bool_text(w.get("hasCastLikeReinterpretation")) if "hasCastLikeReinterpretation" in w else "",
            "has_ptr_arith": _as_bool_text((_metrics(item).get("flags", {}) or {}).get("hasPointerArithmetic", (_metrics(item).get("flags", {}) or {}).get("ptrArith"))) if isinstance(_metrics(item).get("flags"), Mapping) else "",
            "has_byte_view": _as_bool_text(w.get("hasByteView")) if "hasByteView" in w else "",
            "has_byte_view_write": _as_bool_text(w.get("hasByteViewWrite")) if "hasByteViewWrite" in w else "",
            "has_concrete_deref": _as_bool_text(w.get("hasConcreteDeref")) if "hasConcreteDeref" in w else "",
            "has_concrete_write": _as_bool_text(w.get("hasConcreteWrite")) if "hasConcreteWrite" in w else "",
            "has_multi_type_deref": _as_bool_text(w.get("hasMultiTypeDeref")) if "hasMultiTypeDeref" in w else "",
            "has_multi_type_write": _as_bool_text(w.get("hasMultiTypeWrite")) if "hasMultiTypeWrite" in w else "",
            "evidence_source_kind": _evidence_source_kind(evidence),
            "evidence_sources": _evidence_sources(evidence),
            "evidence_flags": _join(evidence.get("flags") or evidence.get("evidenceFlags") or {}),
            "operation_kinds": _operation_kinds_from_evidence(evidence),
        })
        row.update(_paper_fields(item))
        row.update(_callback_mediation_fields(item))
        rows.append(row)
    return rows


def _function_evidence_values(evidence_list: Any, key: str) -> str:
    vals: List[str] = []
    if not isinstance(evidence_list, list):
        return ""
    for ev in evidence_list:
        if not isinstance(ev, Mapping):
            continue
        if key == "entity":
            vals.append(str(ev.get("entity") or ""))
        elif key == "behavior":
            vals.append(str(ev.get("behavior") or ""))
        elif key == "historyShape":
            vals.append(str(ev.get("historyShape") or ""))
        elif key == "historyRisk":
            vals.append(str(ev.get("historyRisk") or ""))
        elif key == "derefTypes":
            vals.extend(str(x) for x in _wild_pointer_metrics(ev).get("derefTypes", []) or [])
        elif key == "writeDerefTypes":
            vals.extend(str(x) for x in _wild_pointer_metrics(ev).get("writeDerefTypes", []) or [])
        elif key == "callbackMediationState":
            vals.append(str(ev.get("callbackMediationState") or ""))
    return _join(sorted({v for v in vals if v}))


def export_function_behavior_rows(summary: Path, root: Path, data: Mapping[str, Any]) -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    common = _common(summary, root, data)
    for item in data.get("functionBehaviorClassifications", []) or []:
        if not isinstance(item, Mapping):
            continue
        ident = item.get("identity") if isinstance(item.get("identity"), Mapping) else {}
        loc = ident.get("location") if isinstance(ident.get("location"), Mapping) else {}
        evidence = item.get("classificationEvidence") if isinstance(item.get("classificationEvidence"), Mapping) else {}
        evidence_list = item.get("evidence") if isinstance(item.get("evidence"), list) else []
        row = dict(common)
        row.update({
            "function": str(item.get("function") or ""),
            "qualified_name": str(ident.get("qualifiedName") or ident.get("name") or item.get("function") or ""),
            "signature_key": str(ident.get("signatureKey") or ""),
            "linkage": str(ident.get("linkage") or ""),
            "definition_file": str(ident.get("sourceFile") or loc.get("file") or ""),
            "definition_line": str(loc.get("line") or ""),
            "definition_column": str(loc.get("column") or ""),
            "function_class": str(item.get("class") or item.get("functionBehavior") or ""),
            "classification_evidence_source_kind": str(item.get("classificationEvidenceSourceKind") or evidence.get("sourceKind") or ""),
            "classification_evidence_sources": _join(item.get("classificationEvidenceSources") or evidence.get("sources") or []),
            "classification_evidence_flags": _join(item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags") or {}),
            "classification_operation_kinds": _join(_nested(evidence, ["operations", "kinds"], [])) or _operation_kinds_from_evidence(evidence),
            "classification_deref_sources": _join(_nested(evidence, ["operations", "derefSources"], [])),
            "classification_write_sources": _join(_nested(evidence, ["operations", "writeSources"], [])),
            "evidence_entities": _function_evidence_values(evidence_list, "entity"),
            "evidence_behaviors": _function_evidence_values(evidence_list, "behavior"),
            "evidence_history_shapes": _function_evidence_values(evidence_list, "historyShape"),
            "evidence_history_risks": _function_evidence_values(evidence_list, "historyRisk"),
            "evidence_deref_types": _function_evidence_values(evidence_list, "derefTypes"),
            "evidence_write_deref_types": _function_evidence_values(evidence_list, "writeDerefTypes"),
            "evidence_callback_mediation_states": _function_evidence_values(evidence_list, "callbackMediationState"),
            "evidence_field_callback_provider": _as_bool_text((item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags") or {}).get("fieldCallbackProvider")) if isinstance(item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags"), Mapping) else "",
            "evidence_guarded_field_callback_selection": _as_bool_text((item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags") or {}).get("guardedFieldCallbackSelection")) if isinstance(item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags"), Mapping) else "",
            "evidence_resolved_callback_mediation": _as_bool_text((item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags") or {}).get("resolvedCallbackMediation")) if isinstance(item.get("classificationEvidenceFlags") or evidence.get("flags") or item.get("evidenceFlags"), Mapping) else "",
        })
        rows.append(row)
    return rows


def find_summaries(root: Path) -> List[Path]:
    """Return report artifacts, preferring C6g5f4 sidecars over legacy summaries.

    Compact `cast-history-tu-link-summary` files intentionally contain no
    report-level metric/classification tables and must never be exported as
    paper-facing behavior data.
    """
    reports = [
        p for p in root.rglob("*.cast-report.json")
        if p.is_file() and not p.name.startswith("project-summary-index")
    ]
    if reports:
        return sorted(reports)
    return sorted(
        p for p in root.rglob("*.cast-summary.json")
        if p.is_file() and not p.name.startswith("project-summary-index")
    )


def write_csv(path: Path, rows: Sequence[Mapping[str, str]], fields: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "") for k in fields})


def write_jsonl(path: Path, rows: Sequence[Mapping[str, str]], fields: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for row in rows:
            obj = {k: row.get(k, "") for k in fields}
            f.write(json.dumps(obj, sort_keys=True, ensure_ascii=False) + "\n")


def export_tables(root: Path, out_dir: Path, *, csv_out: bool = True, jsonl_out: bool = True, quiet: bool = False) -> Dict[str, Any]:
    summaries = find_summaries(root)
    rows_by_table: Dict[str, List[Dict[str, str]]] = {name: [] for name in TABLE_NAMES}
    errors: List[str] = []

    for summary in summaries:
        try:
            data = json.loads(summary.read_text(encoding="utf-8"))
        except Exception as exc:  # noqa: BLE001 - this is a CLI validator/exporter.
            errors.append(f"{summary}: failed to parse JSON: {exc}")
            continue
        if not isinstance(data, Mapping):
            errors.append(f"{summary}: top-level JSON is not an object")
            continue
        if data.get("schemaName") == "cast-history-tu-link-summary":
            errors.append(
                f"{summary}: compact link summary is not a report artifact; "
                "rerun with SUMMARY_REPORTS=1 or select *.cast-report.json"
            )
            continue
        rows_by_table["cast_history_rows"].extend(export_cast_history_rows(summary, root, data))
        rows_by_table["pointer_operation_rows"].extend(export_pointer_operation_rows(summary, root, data))
        rows_by_table["parameter_behavior_rows"].extend(export_parameter_behavior_rows(summary, root, data))
        rows_by_table["function_behavior_rows"].extend(export_function_behavior_rows(summary, root, data))

    out_dir.mkdir(parents=True, exist_ok=True)
    for table, rows in rows_by_table.items():
        fields = FIELDS_BY_TABLE[table]
        if csv_out:
            write_csv(out_dir / f"{table}.csv", rows, fields)
        if jsonl_out:
            write_jsonl(out_dir / f"{table}.jsonl", rows, fields)

    row_counts = {table: len(rows) for table, rows in rows_by_table.items()}
    project_counts = Counter()
    for rows in rows_by_table.values():
        for row in rows:
            if row.get("project"):
                project_counts[row["project"]] += 1

    manifest = {
        "schema": "cast-history-c4r3-behavior-table-export",
        "version": VERSION,
        "generatedAtUTC": datetime.now(timezone.utc).isoformat(),
        "root": root.as_posix(),
        "outDir": out_dir.as_posix(),
        "numSummaryFiles": len(summaries),
        "summaryFiles": [_rel(p, root) for p in summaries],
        "tables": row_counts,
        "projectsSeen": sorted(project_counts),
        "errors": errors,
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")

    if not quiet:
        print(f"C4R3 export: summaries={len(summaries)} errors={len(errors)} out={out_dir}")
        for table in TABLE_NAMES:
            print(f"  {table}: {row_counts[table]}")
        if errors:
            print("Errors:", file=sys.stderr)
            for e in errors:
                print(f"  {e}", file=sys.stderr)
    return manifest


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export normalized cast-history behavior tables from C6g5f4 *.cast-report.json files.")
    parser.add_argument("--version", action="store_true", help="Print script version and exit.")
    parser.add_argument("--root", type=Path, help="Root directory containing one or more analyzer output directories.")
    parser.add_argument("--out-dir", type=Path, help="Directory where CSV/JSONL tables should be written.")
    parser.add_argument("--csv-only", action="store_true", help="Write only CSV tables, not JSONL.")
    parser.add_argument("--jsonl-only", action="store_true", help="Write only JSONL tables, not CSV.")
    parser.add_argument("--quiet", action="store_true", help="Suppress summary output.")
    args = parser.parse_args(argv)
    if args.version:
        print(f"export_behavior_tables.py {VERSION}")
        raise SystemExit(0)
    if args.root is None:
        parser.error("--root is required unless --version is used")
    if args.out_dir is None:
        parser.error("--out-dir is required unless --version is used")
    if args.csv_only and args.jsonl_only:
        parser.error("--csv-only and --jsonl-only cannot both be used")
    return args


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    root = args.root.resolve()
    out_dir = args.out_dir.resolve()
    if not root.exists() or not root.is_dir():
        print(f"error: --root does not exist or is not a directory: {root}", file=sys.stderr)
        return 2
    csv_out = not args.jsonl_only
    jsonl_out = not args.csv_only
    manifest = export_tables(root, out_dir, csv_out=csv_out, jsonl_out=jsonl_out, quiet=args.quiet)
    return 1 if manifest.get("errors") else 0


if __name__ == "__main__":
    raise SystemExit(main())

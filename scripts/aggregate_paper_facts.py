#!/usr/bin/env python3
"""Aggregate C6g5f4 compact paper-facts sidecars into auditable tables.

C6g5f4 emits one `*.cast-paper-facts.json` sidecar per translation unit.  The
sidecar deliberately avoids rendered cast histories and instead stores stable
subject IDs, compact history metrics, pointer-operation facts, refinement
facts, and callback-use relations.  This script deduplicates/merges those rows
within each project and emits CSV/JSONL/LaTeX artifacts for the paper's history
and pointer-operation tables.

Optional alignment CSV inputs add the externally produced 3C and GenC2Rust
outcomes without making the analyzer claim those tools' results.

Examples:
  scripts/aggregate_paper_facts.py \
      --root test-output/bench-genc2rust \
      --out-dir test-output/paper-facts-tables

  scripts/aggregate_paper_facts.py \
      --root test-output/corpus \
      --out-dir test-output/paper-facts-tables \
      --three-c-alignment alignments/three_c_alignment.csv \
      --genc2rust-alignment alignments/genc2rust_alignment.csv
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, Iterator, List, Mapping, MutableMapping, Optional, Sequence, Tuple

SCHEMA_NAME = "cast-history-tu-paper-facts"
SUPPORTED_SCHEMA_VERSIONS = {1, 2, 3, 4, 5, 6, 7, 8, 9}
VERSION = "v9-c6g5f4j-paper-facts-aggregation"

PRIMARY_CATEGORIES = (
    "OpaqueTransportOrViewOnly",
    "MonomorphicActiveSpecialization",
    "ByteOrientedActiveAccess",
    "GuardedActiveReinterpretation",
    "UnguardedMultiTargetActiveReinterpretation",
    "MixedOrProvenanceSensitive",
    "ExternalUnknown",
)

THREE_C_ALIGNMENT_FIELDS = (
    "project", "subject_id", "three_c_subject_id", "three_c_wild",
    "alignment_confidence", "alignment_reason",
)
GENC2RUST_ALIGNMENT_FIELDS = (
    "project", "subject_id", "genc2rust_subject_id", "status",
    "alignment_confidence", "alignment_reason",
)

SUBJECT_FIELDS = (
    "project", "paper_facts_file", "subject_id", "root_subject_id", "entity",
    "subject_kind", "source_file", "source_line", "source_column",
    "function_qualified_name", "function_signature_key", "declared_type", "declared_canonical_type",
    "outer_pointer_kind", "outer_void_pointer",
    "field_storage_scope", "field_storage_base_entity", "field_storage_base_subject_id",
    "field_storage_location_file", "field_storage_location_line", "field_storage_location_column",
    "field_storage_path", "field_storage_declaring_field", "field_storage_declaring_record",
    "field_storage_declaring_type", "field_storage_declaring_canonical_type",
    "field_storage_declaring_file", "field_storage_declaring_line", "field_storage_declaring_column",
    "has_final_history", "canonical_history_id",
    "callback_mediation_state", "callback_is_callback", "callback_resolved_target",
    "callback_field_carrier", "callback_guarded_selection",
    "semantic_mediation_kind", "semantic_mediation_family",
    "history_shape", "history_risk", "history_depth", "history_parallel",
    "history_guarded", "history_pointer_arithmetic", "history_higher_order",
    "history_return_alias", "primary_category", "behavior",
    "transport_only", "deref_read", "deref_write_or_readwrite",
    "pointer_arithmetic", "array_index", "array_index_read", "array_index_write",
    "array_index_readwrite", "array_index_direction_ambiguous",
    "field_mediated_active", "callback_mediated_active",
    "return_alias_mediated_active", "external_escape",
    "active_concrete_types", "active_byte_types", "active_write_concrete_types",
    "active_guards", "guarded_active_multi_target",
    "unguarded_active_multi_target", "incompatible_active_writes",
    "incompatible_write_type_pairs", "integer_pointer_flow",
    "byte_oriented_active_access", "operation_kinds",
)

HISTORY_SUMMARY_FIELDS = (
    "project", "subjects", "histories", "median_depth", "max_depth",
    "parallel", "guarded", "ptr_arith", "higher_order", "return_alias",
)

OPERATION_SUMMARY_FIELDS = (
    "project", "subjects", "transport_only", "deref_read", "deref_write_rw",
    "ptr_arith", "field_active", "callback_active", "return_active",
    "external_escape", "array_index", "array_index_read", "array_index_write",
    "array_index_readwrite", "array_index_direction_ambiguous",
)

CALLBACK_MEDIATION_SUMMARY_FIELDS = (
    "project", "subjects", "formal_callback_parameter", "unresolved_callback",
    "field_callback", "resolved_callback", "resolved_field_callback",
    "guarded_resolved_field_callback", "callback_subjects",
    "resolved_callback_subjects", "field_callback_subjects",
    "guarded_callback_subjects",
)

CALLBACK_FIELDS = (
    "project", "paper_facts_file", "callback_use_id", "subject_id", "entity",
    "callsite_file", "callsite_line", "callsite_column", "target_functions",
    "callee_expr", "guard", "operation_kind", "active_operation",
    "known_function_pointer_target", "guarded_target_set",
    "higher_order_callback_parameter", "field_function_pointer_target",
    "return_alias_continuation",
)


def bool_text(value: Any) -> str:
    return "yes" if value is True else "no" if value is False else ""


def bool_value(value: Any) -> bool:
    return value is True or (isinstance(value, str) and value.strip().lower() in {"1", "true", "yes", "y"})


def latex_escape(value: Any) -> str:
    text = str(value)
    replacements = {
        "\\": r"\textbackslash{}", "&": r"\&", "%": r"\%", "$": r"\$",
        "#": r"\#", "_": r"\_", "{": r"\{", "}": r"\}",
        "~": r"\textasciitilde{}", "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(ch, ch) for ch in text)


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            normalized: Dict[str, Any] = {}
            for field in fields:
                value = row.get(field, "")
                normalized[field] = bool_text(value) if isinstance(value, bool) else value
            writer.writerow(normalized)


def write_jsonl(path: Path, rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")


def as_obj(value: Any) -> Mapping[str, Any]:
    return value if isinstance(value, Mapping) else {}


def as_list(value: Any) -> List[Any]:
    return list(value) if isinstance(value, list) else []


def source_fields(value: Any) -> Tuple[str, int, int]:
    obj = as_obj(value)
    file_name = str(obj.get("file") or "")
    line = obj.get("line")
    column = obj.get("column")
    return file_name, int(line) if isinstance(line, int) else 0, int(column) if isinstance(column, int) else 0


def sorted_unique_strings(value: Any) -> List[str]:
    return sorted({str(item) for item in as_list(value) if str(item)})


def join_strings(value: Any) -> str:
    return " | ".join(sorted_unique_strings(value))


def discover(root: Path) -> List[Path]:
    return sorted(path for path in root.rglob("*.cast-paper-facts.json") if path.is_file())


def project_name(path: Path, root: Path, data: Mapping[str, Any]) -> str:
    """Infer a project ID from common C6g5f runner layouts.

    Expected layouts include `<root>/<project>/paper-facts/*.json`, C1/round
    staging directories, and a direct per-project root.  Users can override
    this with `--project-name` for a single-project input tree.
    """
    explicit = str(as_obj(data.get("project")).get("id") or "")
    if explicit:
        return explicit
    try:
        parts = path.relative_to(root).parts
    except ValueError:
        return path.parent.name
    markers = {
        "paper-facts", "c1-paper-facts", "reports", "summaries", "rounds",
        "import-round-01-paper-facts", "import-round-02-paper-facts",
    }
    for index, part in enumerate(parts):
        if part in markers and index > 0:
            return parts[index - 1]
    return parts[0] if len(parts) > 1 else path.parent.name


def valid_document(path: Path, data: Mapping[str, Any]) -> List[str]:
    errors: List[str] = []
    if data.get("schemaName") != SCHEMA_NAME:
        errors.append(f"{path}: schemaName must be {SCHEMA_NAME!r}")
    if data.get("schemaVersion") not in SUPPORTED_SCHEMA_VERSIONS:
        errors.append(f"{path}: schemaVersion must be one of {sorted(SUPPORTED_SCHEMA_VERSIONS)}")
    if not isinstance(data.get("tu"), Mapping):
        errors.append(f"{path}: tu must be an object")
    if not isinstance(data.get("coverage"), Mapping):
        errors.append(f"{path}: coverage must be an object")
    if not isinstance(data.get("subjects"), list):
        errors.append(f"{path}: subjects must be an array")
    if not isinstance(data.get("callbackUseRecords"), list):
        errors.append(f"{path}: callbackUseRecords must be an array")
    return errors


def category_from_facts(subject: Mapping[str, Any]) -> str:
    """Recompute the documented priority order after TU-duplicate merging."""
    ops = as_obj(subject.get("operations"))
    facts = as_obj(subject.get("refinementFacts"))
    behavior = str(subject.get("behavior") or "")
    if bool_value(ops.get("externalEscape")) or behavior == "external-unknown":
        return "ExternalUnknown"
    if (bool_value(facts.get("integerPointerFlow")) or
            bool_value(facts.get("incompatibleActiveWrites")) or
            behavior in {"dangerous-provenance", "unknown"}):
        return "MixedOrProvenanceSensitive"
    if bool_value(facts.get("unguardedActiveMultiTarget")):
        return "UnguardedMultiTargetActiveReinterpretation"
    if bool_value(facts.get("guardedActiveMultiTarget")):
        return "GuardedActiveReinterpretation"
    if bool_value(facts.get("byteOrientedActiveAccess")):
        return "ByteOrientedActiveAccess"
    if len(sorted_unique_strings(facts.get("activeConcreteTypes"))) == 1:
        return "MonomorphicActiveSpecialization"
    return "OpaqueTransportOrViewOnly"


def merge_bool(obj: MutableMapping[str, Any], field: str, value: Any) -> None:
    obj[field] = bool_value(obj.get(field)) or bool_value(value)


def union_list(obj: MutableMapping[str, Any], field: str, value: Any) -> None:
    merged = sorted({*sorted_unique_strings(obj.get(field)), *sorted_unique_strings(value)})
    obj[field] = merged


def history_key(instance: Mapping[str, Any]) -> str:
    return str(instance.get("historyId") or json.dumps(instance, sort_keys=True))


def evidence_key(value: Mapping[str, Any]) -> str:
    site = as_obj(value.get("operationSite"))
    return "|".join([
        str(value.get("pointerType") or ""), str(value.get("operationKind") or ""),
        str(value.get("guard") or ""), str(site.get("file") or ""),
        str(site.get("line") or ""), str(site.get("column") or ""),
    ])


def merge_subject(existing: MutableMapping[str, Any], incoming: Mapping[str, Any], errors: List[str], context: str) -> None:
    """Losslessly merge duplicate source-located subjects from repeated headers."""
    identity_fields = ("entity", "subjectKind", "rootSubjectId")
    for field in identity_fields:
        left = str(existing.get(field) or "")
        right = str(incoming.get(field) or "")
        if left and right and left != right:
            errors.append(f"{context}: duplicate subjectId has conflicting {field}: {left!r} != {right!r}")

    merge_bool(existing, "hasFinalHistory", incoming.get("hasFinalHistory"))
    if not existing.get("canonicalHistoryId") and incoming.get("canonicalHistoryId"):
        existing["canonicalHistoryId"] = incoming["canonicalHistoryId"]

    # Prefer a final/history-bearing representation for non-boolean summary data.
    if not as_obj(existing.get("history")) and as_obj(incoming.get("history")):
        existing["history"] = dict(as_obj(incoming.get("history")))

    existing_instances = {history_key(as_obj(item)): dict(as_obj(item)) for item in as_list(existing.get("historyInstances"))}
    for item in as_list(incoming.get("historyInstances")):
        candidate = as_obj(item)
        if candidate:
            existing_instances.setdefault(history_key(candidate), dict(candidate))
    existing["historyInstances"] = [existing_instances[key] for key in sorted(existing_instances)]

    existing_ops = dict(as_obj(existing.get("operations")))
    incoming_ops = as_obj(incoming.get("operations"))
    for field, value in incoming_ops.items():
        if isinstance(value, bool):
            merge_bool(existing_ops, field, value)
        elif isinstance(value, list):
            union_list(existing_ops, field, value)
        elif field not in existing_ops or not existing_ops[field]:
            existing_ops[field] = value
    existing["operations"] = existing_ops

    existing_facts = dict(as_obj(existing.get("refinementFacts")))
    incoming_facts = as_obj(incoming.get("refinementFacts"))
    for field, value in incoming_facts.items():
        if isinstance(value, bool):
            merge_bool(existing_facts, field, value)
        elif isinstance(value, list) and field != "activeTypeEvidence":
            union_list(existing_facts, field, value)
        elif field not in existing_facts or not existing_facts[field]:
            existing_facts[field] = value
    evidence: Dict[str, Mapping[str, Any]] = {}
    for item in as_list(existing_facts.get("activeTypeEvidence")) + as_list(incoming_facts.get("activeTypeEvidence")):
        obj = as_obj(item)
        if obj:
            evidence.setdefault(evidence_key(obj), dict(obj))
    existing_facts["activeTypeEvidence"] = [evidence[key] for key in sorted(evidence)]
    existing["refinementFacts"] = existing_facts

    # Callback mediation is a classification-level fact.  Preserve a concrete
    # resolved state over a legacy/missing state and OR its stable flags when
    # repeated header sidecars contribute the same source subject.
    existing_callback = dict(as_obj(existing.get("callbackMediation")))
    incoming_callback = as_obj(incoming.get("callbackMediation"))
    if incoming_callback:
        # Prefer a concrete/field/guarded disposition over a weaker legacy or
        # generic state when repeated header sidecars represent the same source
        # subject.  The flags below remain OR-merged so the audit trail is not
        # lost.
        state_rank = {
            "": 0, "Unknown": 0, "None": 1,
            "UnresolvedCallback": 2, "FormalCallbackParameter": 3,
            "FieldCallback": 4, "ResolvedCallback": 5,
            "ResolvedFieldCallback": 6,
            "GuardedResolvedFieldCallback": 7,
        }
        old_state = str(existing_callback.get("state") or "")
        new_state = str(incoming_callback.get("state") or "")
        if state_rank.get(new_state, 0) > state_rank.get(old_state, 0):
            existing_callback["state"] = new_state
        existing_flags = dict(as_obj(existing_callback.get("flags")))
        for field, value in as_obj(incoming_callback.get("flags")).items():
            if isinstance(value, bool):
                merge_bool(existing_flags, field, value)
            elif field not in existing_flags:
                existing_flags[field] = value
        existing_callback["flags"] = existing_flags
        existing["callbackMediation"] = existing_callback

    # Field-storage metadata names the same resolved instance and its
    # declaring aggregate field. It is reporting-only but must not silently
    # conflict when a shared header contributes duplicate sidecars.
    existing_field_storage = dict(as_obj(existing.get("fieldStorage")))
    incoming_field_storage = as_obj(incoming.get("fieldStorage"))
    if incoming_field_storage:
        if existing_field_storage:
            for field in ("classificationScope", "baseEntity", "baseSubjectId", "fieldPathText"):
                left = str(existing_field_storage.get(field) or "")
                right = str(incoming_field_storage.get(field) or "")
                if left and right and left != right:
                    errors.append(f"{context}: duplicate field-storage subject conflicts on {field}: {left!r} != {right!r}")
        else:
            existing_field_storage = dict(incoming_field_storage)
        existing["fieldStorage"] = existing_field_storage

    # Retain a visible record of all sidecars that contributed evidence.
    source_files = sorted({*sorted_unique_strings(existing.get("_sourceFiles")), *sorted_unique_strings(incoming.get("_sourceFiles"))})
    existing["_sourceFiles"] = source_files
    existing["primaryCategory"] = category_from_facts(existing)


def flatten_subject(project: str, path: Path, subject: Mapping[str, Any]) -> Dict[str, Any]:
    loc = as_obj(subject.get("location"))
    function = as_obj(subject.get("function"))
    declared = as_obj(subject.get("declaredType"))
    history = as_obj(subject.get("history"))
    ops = as_obj(subject.get("operations"))
    facts = as_obj(subject.get("refinementFacts"))
    callback_mediation = as_obj(subject.get("callbackMediation"))
    callback_flags = as_obj(callback_mediation.get("flags"))
    semantic_mediation = as_obj(subject.get("semanticMediation"))
    field_storage = as_obj(subject.get("fieldStorage"))
    field_storage_location = as_obj(field_storage.get("storageLocation"))
    declaring_field = as_obj(field_storage.get("declaringField"))
    declaring_location = as_obj(declaring_field.get("location"))
    return {
        "project": project,
        "paper_facts_file": join_strings(subject.get("_sourceFiles")) or path.as_posix(),
        "subject_id": subject.get("subjectId", ""),
        "root_subject_id": subject.get("rootSubjectId", ""),
        "entity": subject.get("entity", ""),
        "subject_kind": subject.get("subjectKind", ""),
        "source_file": loc.get("file", ""), "source_line": loc.get("line", ""),
        "source_column": loc.get("column", ""),
        "function_qualified_name": function.get("qualifiedName", ""),
        "function_signature_key": function.get("signatureKey", ""),
        "declared_type": declared.get("text", ""),
        "declared_canonical_type": declared.get("canonicalText", ""),
        "outer_pointer_kind": declared.get("outerPointerKind", "unknown"),
        "outer_void_pointer": bool_value(declared.get("outerVoidPointer")),
        "field_storage_scope": field_storage.get("classificationScope", ""),
        "field_storage_base_entity": field_storage.get("baseEntity", ""),
        "field_storage_base_subject_id": field_storage.get("baseSubjectId", ""),
        "field_storage_location_file": field_storage_location.get("file", ""),
        "field_storage_location_line": field_storage_location.get("line", ""),
        "field_storage_location_column": field_storage_location.get("column", ""),
        "field_storage_path": field_storage.get("fieldPathText", ""),
        "field_storage_declaring_field": declaring_field.get("qualifiedName", ""),
        "field_storage_declaring_record": declaring_field.get("recordName", ""),
        "field_storage_declaring_type": declaring_field.get("type", ""),
        "field_storage_declaring_canonical_type": declaring_field.get("canonicalType", ""),
        "field_storage_declaring_file": declaring_location.get("file", ""),
        "field_storage_declaring_line": declaring_location.get("line", ""),
        "field_storage_declaring_column": declaring_location.get("column", ""),
        "callback_mediation_state": callback_mediation.get("state", ""),
        "callback_is_callback": bool_value(callback_flags.get("callback")),
        "callback_resolved_target": bool_value(callback_flags.get("resolvedTarget")),
        "callback_field_carrier": bool_value(callback_flags.get("fieldCarrier")),
        "callback_guarded_selection": bool_value(callback_flags.get("guardedSelection")),
        "semantic_mediation_kind": semantic_mediation.get("kind", ""),
        "semantic_mediation_family": semantic_mediation.get("semanticFamily", ""),
        "has_final_history": bool_value(subject.get("hasFinalHistory")),
        "canonical_history_id": subject.get("canonicalHistoryId", ""),
        "history_shape": history.get("shape", ""), "history_risk": history.get("risk", ""),
        "history_depth": history.get("depth", 0), "history_parallel": bool_value(history.get("parallel")),
        "history_guarded": bool_value(history.get("guarded")),
        "history_pointer_arithmetic": bool_value(history.get("pointerArithmetic")),
        "history_higher_order": bool_value(history.get("higherOrder")),
        "history_return_alias": bool_value(history.get("returnAlias")),
        "primary_category": subject.get("primaryCategory", ""), "behavior": subject.get("behavior", ""),
        "transport_only": bool_value(ops.get("transportOnly")),
        "deref_read": bool_value(ops.get("derefRead")),
        "deref_write_or_readwrite": bool_value(ops.get("derefWriteOrReadWrite")),
        "pointer_arithmetic": bool_value(ops.get("pointerArithmetic")),
        "array_index": bool_value(ops.get("arrayIndex")),
        "array_index_read": bool_value(ops.get("arrayIndexRead")),
        "array_index_write": bool_value(ops.get("arrayIndexWrite")),
        "array_index_readwrite": bool_value(ops.get("arrayIndexReadWrite")),
        "array_index_direction_ambiguous": bool_value(ops.get("arrayIndexDirectionAmbiguous")),
        "field_mediated_active": bool_value(ops.get("fieldMediatedActive")),
        "callback_mediated_active": bool_value(ops.get("callbackMediatedActive")),
        "return_alias_mediated_active": bool_value(ops.get("returnAliasMediatedActive")),
        "external_escape": bool_value(ops.get("externalEscape")),
        "active_concrete_types": join_strings(facts.get("activeConcreteTypes")),
        "active_byte_types": join_strings(facts.get("activeByteTypes")),
        "active_write_concrete_types": join_strings(facts.get("activeWriteConcreteTypes")),
        "active_guards": join_strings(facts.get("activeGuards")),
        "guarded_active_multi_target": bool_value(facts.get("guardedActiveMultiTarget")),
        "unguarded_active_multi_target": bool_value(facts.get("unguardedActiveMultiTarget")),
        "incompatible_active_writes": bool_value(facts.get("incompatibleActiveWrites")),
        "incompatible_write_type_pairs": join_strings(facts.get("incompatibleWriteTypePairs")),
        "integer_pointer_flow": bool_value(facts.get("integerPointerFlow")),
        "byte_oriented_active_access": bool_value(facts.get("byteOrientedActiveAccess")),
        "operation_kinds": join_strings(ops.get("operationKinds")),
    }


def flatten_callback(project: str, path: Path, record: Mapping[str, Any]) -> Dict[str, Any]:
    site = as_obj(record.get("callSite"))
    flags = as_obj(record.get("flags"))
    return {
        "project": project, "paper_facts_file": path.as_posix(),
        "callback_use_id": record.get("callbackUseId", ""),
        "subject_id": record.get("subjectId", ""), "entity": record.get("entity", ""),
        "callsite_file": site.get("file", ""), "callsite_line": site.get("line", ""),
        "callsite_column": site.get("column", ""),
        "target_functions": join_strings(record.get("targetFunctions")),
        "callee_expr": record.get("calleeExpr", ""), "guard": record.get("guard", ""),
        "operation_kind": record.get("operationKind", ""),
        "active_operation": bool_value(record.get("activeOperation")),
        "known_function_pointer_target": bool_value(flags.get("knownFunctionPointerTarget")),
        "guarded_target_set": bool_value(flags.get("guardedTargetSet")),
        "higher_order_callback_parameter": bool_value(flags.get("higherOrderCallbackParameter")),
        "field_function_pointer_target": bool_value(flags.get("fieldFunctionPointerTarget")),
        "return_alias_continuation": bool_value(flags.get("returnAliasContinuation")),
    }


def median(values: Sequence[int]) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        return float(ordered[middle])
    return (ordered[middle - 1] + ordered[middle]) / 2.0


def history_summary_row(project: str, subjects: Sequence[Mapping[str, Any]]) -> Dict[str, Any]:
    depths: List[int] = []
    history_count = 0
    for subject in subjects:
        instances = as_list(subject.get("_historyInstances"))
        final_instances = [as_obj(item) for item in instances if bool_value(as_obj(item).get("hasFinalHistory"))]
        history_count += len(final_instances)
        for instance in final_instances:
            metric = as_obj(instance.get("metrics"))
            value = metric.get("depth")
            if isinstance(value, int):
                depths.append(value)
    return {
        "project": project,
        "subjects": len(subjects),
        "histories": history_count,
        "median_depth": median(depths),
        "max_depth": max(depths) if depths else 0,
        "parallel": sum(bool_value(row.get("history_parallel")) for row in subjects),
        "guarded": sum(bool_value(row.get("history_guarded")) for row in subjects),
        "ptr_arith": sum(bool_value(row.get("history_pointer_arithmetic")) for row in subjects),
        "higher_order": sum(bool_value(row.get("history_higher_order")) for row in subjects),
        "return_alias": sum(bool_value(row.get("history_return_alias")) for row in subjects),
    }


def summarize_histories(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    by_project: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    active_subjects: List[Mapping[str, Any]] = []
    for row in rows:
        if bool_value(row.get("has_final_history")):
            by_project[str(row["project"])].append(row)
            active_subjects.append(row)
    result = [history_summary_row(project, by_project[project]) for project in sorted(by_project)]
    if result:
        result.append(history_summary_row("Total", active_subjects))
    return result

def operation_summary_row(project: str, subjects: Sequence[Mapping[str, Any]]) -> Dict[str, Any]:
    flags = {
        "transport_only": "transport_only", "deref_read": "deref_read",
        "deref_write_rw": "deref_write_or_readwrite", "ptr_arith": "pointer_arithmetic",
        "field_active": "field_mediated_active", "callback_active": "callback_mediated_active",
        "return_active": "return_alias_mediated_active", "external_escape": "external_escape",
        "array_index": "array_index", "array_index_read": "array_index_read",
        "array_index_write": "array_index_write", "array_index_readwrite": "array_index_readwrite",
        "array_index_direction_ambiguous": "array_index_direction_ambiguous",
    }
    row: Dict[str, Any] = {"project": project, "subjects": len(subjects)}
    for output, source in flags.items():
        row[output] = sum(bool_value(subject.get(source)) for subject in subjects)
    return row


def summarize_operations(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    by_project: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    active_subjects: List[Mapping[str, Any]] = []
    for row in rows:
        if bool_value(row.get("has_final_history")):
            by_project[str(row["project"])].append(row)
            active_subjects.append(row)
    result = [operation_summary_row(project, by_project[project]) for project in sorted(by_project)]
    if result:
        result.append(operation_summary_row("Total", active_subjects))
    return result

def callback_mediation_summary_row(project: str, subjects: Sequence[Mapping[str, Any]]) -> Dict[str, Any]:
    states = Counter(str(subject.get("callback_mediation_state") or "") for subject in subjects)
    return {
        "project": project,
        "subjects": len(subjects),
        "formal_callback_parameter": states["FormalCallbackParameter"],
        "unresolved_callback": states["UnresolvedCallback"],
        "field_callback": states["FieldCallback"],
        "resolved_callback": states["ResolvedCallback"],
        "resolved_field_callback": states["ResolvedFieldCallback"],
        "guarded_resolved_field_callback": states["GuardedResolvedFieldCallback"],
        "callback_subjects": sum(bool_value(subject.get("callback_is_callback")) for subject in subjects),
        "resolved_callback_subjects": sum(bool_value(subject.get("callback_resolved_target")) for subject in subjects),
        "field_callback_subjects": sum(bool_value(subject.get("callback_field_carrier")) for subject in subjects),
        "guarded_callback_subjects": sum(bool_value(subject.get("callback_guarded_selection")) for subject in subjects),
    }


def summarize_callback_mediation(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    by_project: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    active_subjects: List[Mapping[str, Any]] = []
    for row in rows:
        if bool_value(row.get("has_final_history")):
            by_project[str(row["project"])].append(row)
            active_subjects.append(row)
    result = [callback_mediation_summary_row(project, by_project[project])
              for project in sorted(by_project)]
    if result:
        result.append(callback_mediation_summary_row("Total", active_subjects))
    return result

def latex_history_table(rows: Sequence[Mapping[str, Any]]) -> str:
    project_rows = [row for row in rows if row.get("project") != "Total"]
    total = next((row for row in rows if row.get("project") == "Total"), None)
    lines = [
        r"\begin{table*}[t]", r"\centering",
        r"\caption{Cast-history structure by project.}",
        r"\label{tab:history-structure}",
        r"\begin{tabular}{lrrrrrrrrr}", r"\toprule",
        r"Project & Subjects & Histories & Median depth & Max depth & Parallel & Guarded & Ptr-arith & Higher-order & Return-alias \\",
        r"\midrule",
    ]
    for row in project_rows:
        med = row["median_depth"]
        med_text = str(int(med)) if float(med).is_integer() else f"{med:.1f}"
        lines.append(
            f"{latex_escape(row['project'])} & {row['subjects']} & {row['histories']} & {med_text} & {row['max_depth']} & "
            f"{row['parallel']} & {row['guarded']} & {row['ptr_arith']} & {row['higher_order']} & {row['return_alias']} \\")
    if total is not None:
        med = total["median_depth"]
        med_text = str(int(med)) if float(med).is_integer() else f"{med:.1f}"
        lines.extend([
            r"\midrule",
            f"Total & {total['subjects']} & {total['histories']} & {med_text} & {total['max_depth']} & {total['parallel']} & {total['guarded']} & {total['ptr_arith']} & {total['higher_order']} & {total['return_alias']} \\",
        ])
    lines.extend([r"\bottomrule", r"\end{tabular}", r"\end{table}", ""])
    return "\n".join(lines)

def latex_operation_table(rows: Sequence[Mapping[str, Any]]) -> str:
    project_rows = [row for row in rows if row.get("project") != "Total"]
    total = next((row for row in rows if row.get("project") == "Total"), None)
    lines = [
        r"\begin{table*}[t]", r"\centering",
        r"\caption{Pointer-operation and mediation evidence for cast-history subjects. Operation columns may overlap.}",
        r"\label{tab:pointer-operations}",
        r"\begin{tabular}{lrrrrrrrrr}", r"\toprule",
        r"Project & Subjects & Transport-only & Deref-read & Deref-write/RW & Ptr-arith & Field-active & Callback-active & Return-active & External escape \\",
        r"\midrule",
    ]
    for row in project_rows:
        lines.append(
            f"{latex_escape(row['project'])} & {row['subjects']} & {row['transport_only']} & {row['deref_read']} & "
            f"{row['deref_write_rw']} & {row['ptr_arith']} & {row['field_active']} & {row['callback_active']} & "
            f"{row['return_active']} & {row['external_escape']} \\")
    if total is not None:
        lines.extend([
            r"\midrule",
            f"Total & {total['subjects']} & {total['transport_only']} & {total['deref_read']} & {total['deref_write_rw']} & {total['ptr_arith']} & {total['field_active']} & {total['callback_active']} & {total['return_active']} & {total['external_escape']} \\",
        ])
    lines.extend([r"\bottomrule", r"\end{tabular}", r"\end{table}", ""])
    return "\n".join(lines)

def read_alignment(path: Path, required: Sequence[str], errors: List[str]) -> List[Dict[str, str]]:
    try:
        with path.open(encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            missing = [field for field in required if field not in (reader.fieldnames or [])]
            if missing:
                errors.append(f"{path}: missing required columns: {', '.join(missing)}")
                return []
            return [{field: str(row.get(field) or "").strip() for field in (reader.fieldnames or [])} for row in reader]
    except OSError as exc:
        errors.append(f"{path}: cannot read alignment CSV: {exc}")
        return []


def write_alignment_templates(out_dir: Path, rows: Sequence[Mapping[str, Any]]) -> None:
    template_rows = [
        {"project": row["project"], "subject_id": row["subject_id"],
         "three_c_subject_id": "", "three_c_wild": "", "alignment_confidence": "", "alignment_reason": ""}
        for row in rows
    ]
    write_csv(out_dir / "three_c_alignment_template.csv", THREE_C_ALIGNMENT_FIELDS, template_rows)
    template_rows = [
        {"project": row["project"], "subject_id": row["subject_id"],
         "genc2rust_subject_id": "", "status": "", "alignment_confidence": "", "alignment_reason": ""}
        for row in rows if bool_value(row.get("outer_void_pointer"))
    ]
    write_csv(out_dir / "genc2rust_alignment_template.csv", GENC2RUST_ALIGNMENT_FIELDS, template_rows)


def make_3c_outputs(subject_rows: Sequence[Mapping[str, Any]], alignment: Sequence[Mapping[str, str]], out_dir: Path, errors: List[str]) -> None:
    subject_by_key = {(str(row["project"]), str(row["subject_id"])): row for row in subject_rows}
    aligned: List[Mapping[str, Any]] = []
    seen: set[Tuple[str, str]] = set()
    for row in alignment:
        if not bool_value(row.get("three_c_wild")):
            continue
        key = (row.get("project", ""), row.get("subject_id", ""))
        if key in seen:
            errors.append(f"3C alignment: duplicate wild subject alignment {key}")
            continue
        seen.add(key)
        subject = subject_by_key.get(key)
        if subject is None:
            errors.append(f"3C alignment: no paper-facts subject for {key}")
            continue
        aligned.append(subject)

    category_rows: List[Dict[str, Any]] = []
    total = len(aligned)
    counts = Counter(str(row.get("primary_category") or "") for row in aligned)
    interpretations = {
        "OpaqueTransportOrViewOnly": "Structured / not actively wild",
        "MonomorphicActiveSpecialization": "Structured / bounded",
        "ByteOrientedActiveAccess": "Translation-sensitive",
        "GuardedActiveReinterpretation": "Structured / translation-sensitive",
        "UnguardedMultiTargetActiveReinterpretation": "Semantically wild / high-risk",
        "MixedOrProvenanceSensitive": "Semantically wild / high-risk",
        "ExternalUnknown": "Unknown",
    }
    for category in PRIMARY_CATEGORIES:
        count = counts[category]
        category_rows.append({"category": category, "count": count,
                              "fraction": (count / total) if total else 0.0,
                              "interpretation": interpretations[category]})
    write_csv(out_dir / "three_c_wild_refinement.csv", ("category", "count", "fraction", "interpretation"), category_rows)

    concentration: Dict[str, Counter[str]] = defaultdict(Counter)
    for subject in aligned:
        category = str(subject.get("primary_category") or "")
        if category in {"OpaqueTransportOrViewOnly", "MonomorphicActiveSpecialization"}:
            family = "bounded_structured"
        elif category in {"ByteOrientedActiveAccess", "GuardedActiveReinterpretation"}:
            family = "translation_sensitive"
        else:
            family = "unknown_high_risk"
        concentration[str(subject["project"])][family] += 1
    rows = []
    for project in sorted(concentration):
        counts_for_project = concentration[project]
        rows.append({"project": project, "three_c_wild_subjects": sum(counts_for_project.values()),
                     "bounded_structured": counts_for_project["bounded_structured"],
                     "translation_sensitive": counts_for_project["translation_sensitive"],
                     "unknown_high_risk": counts_for_project["unknown_high_risk"]})
    write_csv(out_dir / "three_c_wild_project_concentration.csv",
              ("project", "three_c_wild_subjects", "bounded_structured", "translation_sensitive", "unknown_high_risk"), rows)


def make_genc2rust_outputs(subject_rows: Sequence[Mapping[str, Any]], callback_rows: Sequence[Mapping[str, Any]], alignment: Sequence[Mapping[str, str]], out_dir: Path, errors: List[str]) -> None:
    statuses = ("Generic", "NonGeneric", "Unsupported")
    normalized_status = {
        "generic": "Generic", "non-generic": "NonGeneric", "nongeneric": "NonGeneric",
        "unsupported": "Unsupported", "unhandled": "Unsupported", "unsupported/unhandled": "Unsupported",
    }
    subject_by_key = {(str(row["project"]), str(row["subject_id"])): row for row in subject_rows}
    aligned: Dict[Tuple[str, str], str] = {}
    for row in alignment:
        key = (row.get("project", ""), row.get("subject_id", ""))
        raw = row.get("status", "").strip().lower()
        status = normalized_status.get(raw)
        if not status:
            errors.append(f"GenC2Rust alignment: unsupported status {row.get('status')!r} for {key}")
            continue
        if key not in subject_by_key:
            errors.append(f"GenC2Rust alignment: no paper-facts subject for {key}")
            continue
        if key in aligned and aligned[key] != status:
            errors.append(f"GenC2Rust alignment: conflicting status for {key}")
            continue
        aligned[key] = status

    table: Dict[str, Counter[str]] = {category: Counter() for category in PRIMARY_CATEGORIES}
    for key, status in aligned.items():
        category = str(subject_by_key[key].get("primary_category") or "")
        if category in table:
            table[category][status] += 1
    rows = []
    for category in PRIMARY_CATEGORIES:
        counts = table[category]
        rows.append({"category": category, "generic": counts["Generic"],
                     "non_generic": counts["NonGeneric"], "unsupported_unhandled": counts["Unsupported"],
                     "total": sum(counts.values())})
    write_csv(out_dir / "genc2rust_cross_tab.csv", ("category", "generic", "non_generic", "unsupported_unhandled", "total"), rows)

    callback_summary: Dict[str, Counter[str]] = defaultdict(Counter)
    for callback in callback_rows:
        key = (str(callback.get("project") or ""), str(callback.get("subject_id") or ""))
        status = aligned.get(key)
        if not status:
            continue
        flags = {
            "KnownFunctionPointerTarget": bool_value(callback.get("known_function_pointer_target")),
            "GuardedCallbackTargetSet": bool_value(callback.get("guarded_target_set")),
            "HigherOrderCallbackParameter": bool_value(callback.get("higher_order_callback_parameter")),
            "FieldFunctionPointerTarget": bool_value(callback.get("field_function_pointer_target")),
            "CallbackWithReturnAliasContinuation": bool_value(callback.get("return_alias_continuation")),
        }
        for pattern, enabled in flags.items():
            if enabled:
                callback_summary[pattern]["records"] += 1
                callback_summary[pattern][status] += 1
                if bool_value(callback.get("active_operation")):
                    callback_summary[pattern]["active_target_operation"] += 1
    rows = []
    for pattern in ("KnownFunctionPointerTarget", "GuardedCallbackTargetSet", "HigherOrderCallbackParameter", "FieldFunctionPointerTarget", "CallbackWithReturnAliasContinuation"):
        counts = callback_summary[pattern]
        rows.append({"callback_use_pattern": pattern, "records": counts["records"],
                     "active_target_operation": counts["active_target_operation"],
                     "generic": counts["Generic"], "non_generic": counts["NonGeneric"],
                     "unsupported_unhandled": counts["Unsupported"]})
    write_csv(out_dir / "genc2rust_callback_use_cross_tab.csv",
              ("callback_use_pattern", "records", "active_target_operation", "generic", "non_generic", "unsupported_unhandled"), rows)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Aggregate C6g5f4 compact cast-history paper-facts sidecars.")
    parser.add_argument("--root", type=Path, required=True, help="Root recursively containing *.cast-paper-facts.json")
    parser.add_argument("--out-dir", type=Path, required=True, help="Directory for project tables, audit rows, and alignment templates")
    parser.add_argument("--project-name", default="", help="Override inferred project name for every discovered sidecar (for a one-project root)")
    parser.add_argument("--three-c-alignment", type=Path, help="Optional CSV using three_c_alignment_template.csv columns")
    parser.add_argument("--genc2rust-alignment", type=Path, help="Optional CSV using genc2rust_alignment_template.csv columns")
    parser.add_argument("--strict", action="store_true", help="Treat malformed sidecars and conflicting duplicate IDs as fatal")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    out_dir = args.out_dir.resolve()
    paths = discover(root)
    if not paths:
        print(f"error: no *.cast-paper-facts.json files under {root}", file=sys.stderr)
        return 2
    out_dir.mkdir(parents=True, exist_ok=True)

    errors: List[str] = []
    subjects: Dict[Tuple[str, str], Dict[str, Any]] = {}
    callback_records: Dict[Tuple[str, str], Dict[str, Any]] = {}
    coverage_rows: List[Dict[str, Any]] = []

    for path in paths:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: cannot read JSON: {exc}")
            continue
        if not isinstance(data, Mapping):
            errors.append(f"{path}: top-level JSON must be an object")
            continue
        errors.extend(valid_document(path, data))
        project = args.project_name or project_name(path, root, data)
        coverage = as_obj(data.get("coverage"))
        coverage_rows.append({"project": project, "paper_facts_file": path.as_posix(), **coverage})

        for item in as_list(data.get("subjects")):
            subject = dict(as_obj(item))
            subject_id = str(subject.get("subjectId") or "")
            if not subject_id:
                errors.append(f"{path}: subject row lacks subjectId")
                continue
            subject["_sourceFiles"] = [path.as_posix()]
            key = (project, subject_id)
            existing = subjects.get(key)
            if existing is None:
                subjects[key] = subject
            else:
                merge_subject(existing, subject, errors, f"{path}: {key}")

        for item in as_list(data.get("callbackUseRecords")):
            callback = dict(as_obj(item))
            callback_id = str(callback.get("callbackUseId") or "")
            if not callback_id:
                errors.append(f"{path}: callback-use row lacks callbackUseId")
                continue
            key = (project, callback_id)
            callback["_sourceFiles"] = [path.as_posix()]
            prior = callback_records.get(key)
            if prior is None:
                callback_records[key] = callback
            elif prior != callback:
                # Repeated header callbacks can differ only in provenance detail;
                # retain one row but require the stable relation identity to match.
                prior["_sourceFiles"] = sorted({*sorted_unique_strings(prior.get("_sourceFiles")), path.as_posix()})

    flat_subjects = [flatten_subject(project, Path(subject.get("_sourceFiles", [""])[0]), subject)
                     for (project, _), subject in sorted(subjects.items())]
    for flat, ((project, _), subject) in zip(flat_subjects, sorted(subjects.items())):
        flat["_historyInstances"] = as_list(subject.get("historyInstances"))
    flat_callbacks = [flatten_callback(project, Path(callback.get("_sourceFiles", [""])[0]), callback)
                      for (project, _), callback in sorted(callback_records.items())]

    history_rows = summarize_histories(flat_subjects)
    operation_rows = summarize_operations(flat_subjects)
    callback_mediation_rows = summarize_callback_mediation(flat_subjects)
    write_csv(out_dir / "paper_subject_facts.csv", SUBJECT_FIELDS, flat_subjects)
    write_jsonl(out_dir / "paper_subject_facts.jsonl", flat_subjects)
    write_csv(out_dir / "paper_callback_use_records.csv", CALLBACK_FIELDS, flat_callbacks)
    write_jsonl(out_dir / "paper_callback_use_records.jsonl", flat_callbacks)
    write_csv(out_dir / "paper_facts_coverage.csv", ("project", "paper_facts_file", "subjectRows", "subjectsWithFinalHistory", "callbackUseRecords", "arrayIndexDirectionModel", "declaredOuterPointerKindModel", "callbackMediationModel"), coverage_rows)
    write_csv(out_dir / "history_metrics_by_project.csv", HISTORY_SUMMARY_FIELDS, history_rows)
    write_csv(out_dir / "pointer_operations_by_project.csv", OPERATION_SUMMARY_FIELDS, operation_rows)
    write_csv(out_dir / "callback_mediation_by_project.csv", CALLBACK_MEDIATION_SUMMARY_FIELDS,
              callback_mediation_rows)
    (out_dir / "history_metrics.tex").write_text(latex_history_table(history_rows), encoding="utf-8")
    (out_dir / "pointer_operations.tex").write_text(latex_operation_table(operation_rows), encoding="utf-8")
    write_alignment_templates(out_dir, flat_subjects)

    if args.three_c_alignment:
        alignment = read_alignment(args.three_c_alignment.resolve(), THREE_C_ALIGNMENT_FIELDS, errors)
        make_3c_outputs(flat_subjects, alignment, out_dir, errors)
    if args.genc2rust_alignment:
        alignment = read_alignment(args.genc2rust_alignment.resolve(), GENC2RUST_ALIGNMENT_FIELDS, errors)
        make_genc2rust_outputs(flat_subjects, flat_callbacks, alignment, out_dir, errors)

    manifest = {
        "schemaName": "cast-history-paper-facts-aggregation-manifest",
        "schemaVersion": 1,
        "toolVersion": VERSION,
        "createdAt": datetime.now(timezone.utc).isoformat(),
        "root": root.as_posix(),
        "inputFiles": [path.as_posix() for path in paths],
        "projects": sorted({row["project"] for row in flat_subjects}),
        "subjectRows": len(flat_subjects),
        "subjectsWithFinalHistory": sum(bool_value(row.get("has_final_history")) for row in flat_subjects),
        "callbackUseRecords": len(flat_callbacks),
        "errors": errors,
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for error in errors:
        print(f"warning: {error}", file=sys.stderr)
    if args.strict and errors:
        return 1
    print(f"aggregated {len(paths)} paper-facts sidecars into {len(flat_subjects)} subject rows and {len(flat_callbacks)} callback-use rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

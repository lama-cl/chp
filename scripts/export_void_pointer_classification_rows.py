#!/usr/bin/env python3
"""Create one auditable row per direct ``void *`` declaration.

C6g5f4d deliberately produces complementary aggregates:

* ``pointer_declaration_rows.csv`` enumerates all direct outer pointer
  declarations found by the prevalence pass;
* ``void_pointer_reconciliation_rows.csv`` adds declaration-inventory and
  cast-history coverage status for direct local/parameter ``void *`` values;
* ``paper_subject_facts.csv`` carries the final semantic category and compact
  operation/history facts for emitted cast-history subjects.

This script performs the stable joins among those tables and writes
``void_pointer_classification_rows.csv``.  The output has one row per direct
``void *`` declaration whenever prevalence observed it, plus any
inventory-only local/parameter declaration retained by the reconciliation
input.  Consequently it is a single, reviewable denominator for:

  * direct declaration kind (global / field / return / local / parameter),
  * cast-history coverage status,
  * final semantic category where one was inferred, and
  * operation/history evidence for emitted subjects.

It deliberately does *not* treat ``declared-no-observed-facts`` as proof of
parametricity.  A blank semantic category means that the current analyzer did
not infer a final cast-history category for that declaration.

Example:
  scripts/export_void_pointer_classification_rows.py \\
      --prevalence-rows results/prevalence/pointer_declaration_rows.csv \\
      --reconciliation-rows results/void-pointer-reconciliation/void_pointer_reconciliation_rows.csv \\
      --paper-subject-facts results/paper-facts/paper_subject_facts.csv \\
      --out results/void-pointer-reconciliation/void_pointer_classification_rows.csv \\
      --strict
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

VERSION = "v2-c6g5f4j-void-pointer-classification-rows"

PRIMARY_CATEGORIES = (
    "OpaqueTransportOrViewOnly",
    "MonomorphicActiveSpecialization",
    "ByteOrientedActiveAccess",
    "GuardedActiveReinterpretation",
    "UnguardedMultiTargetActiveReinterpretation",
    "MixedOrProvenanceSensitive",
    "ExternalUnknown",
)

CATEGORY_FAMILIES = {
    "OpaqueTransportOrViewOnly": "Structured / view-only",
    "MonomorphicActiveSpecialization": "Structured / bounded",
    "ByteOrientedActiveAccess": "Translation-sensitive",
    "GuardedActiveReinterpretation": "Translation-sensitive",
    "UnguardedMultiTargetActiveReinterpretation": "Semantically wild / high-risk",
    "MixedOrProvenanceSensitive": "Semantically wild / high-risk",
    "ExternalUnknown": "External unknown",
}

ROW_FIELDS = (
    # Stable declaration identity and direct-declaration prevalence evidence.
    "project",
    "declaration_id",
    "direct_void_pointer_prevalence",
    "prevalence_file",
    "declaration_kind",
    "declaration_name",
    "declared_type",
    "declared_canonical_type",
    "source_file",
    "source_line",
    "source_column",
    # C6g5f4d inventory/reconciliation state.  These are blank / not-applicable
    # for globals, fields, and returns because the inventory is intentionally
    # local/parameter-only.
    "reconciliation_status",
    "subject_id",
    "inventory_subject_kind",
    "inventory_declared_type",
    "inventory_canonical_type",
    "outer_pointer_kind",
    "outer_void_pointer",
    "eligible_for_cast_history_classification",
    "semantic_classification_scope",
    "owner_definition_available",
    "eligibility_exclusion_reason",
    "subject_fact_emitted",
    "has_final_history",
    "classification_status",
    "classification_scope",
    # Final classification for emitted subject facts, if available.
    "semantic_category",
    "semantic_family",
    "category_source",
    "category_consistency",
    "canonical_history_id",
    "history_shape",
    "history_risk",
    "behavior",
    "semantic_mediation_kind",
    "semantic_mediation_family",
    "callback_mediation_state",
    # Compact operation/refinement evidence.  These fields make the table
    # useful for manual adjudication without reopening large cast-report JSON.
    "transport_only",
    "deref_read",
    "deref_write_or_readwrite",
    "pointer_arithmetic",
    "array_index",
    "field_mediated_active",
    "callback_mediated_active",
    "return_alias_mediated_active",
    "external_escape",
    "active_concrete_types",
    "active_byte_types",
    "active_write_concrete_types",
    "active_guards",
    "operation_kinds",
    "paper_facts_file",
)

SUMMARY_FIELDS = (
    "project",
    "direct_void_pointer_declarations",
    "globals",
    "fields",
    "returns",
    "locals",
    "parameters",
    "not_cast_history_eligible",
    "out_of_scope_unavailable_api_parameter",
    "classified",
    "observed_no_final_history",
    "declared_no_observed_facts",
    "final_history_not_emitted",
    "prevalence_only_no_inventory",
    "inventory_only",
    "rows_with_semantic_category",
    *PRIMARY_CATEGORIES,
)


class InputError(ValueError):
    """An input table violates the stable aggregation contract."""


def bool_value(value: Any) -> bool:
    if value is True:
        return True
    if isinstance(value, str):
        return value.strip().lower() in {"1", "true", "yes", "y", "on"}
    return False


def bool_text(value: Any) -> str:
    return "yes" if value is True else "no" if value is False else ""


def integer_text(value: Any) -> str:
    try:
        return str(int(str(value).strip() or "0"))
    except (TypeError, ValueError):
        return "0"


def read_csv(path: Path, required: Iterable[str]) -> List[Dict[str, str]]:
    try:
        with path.open(encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            fields = set(reader.fieldnames or [])
            missing = sorted(set(required) - fields)
            if missing:
                raise InputError(f"{path}: missing required CSV columns: {', '.join(missing)}")
            return [dict(row) for row in reader]
    except OSError as exc:
        raise InputError(f"{path}: cannot read CSV: {exc}") from exc


def unique_index(
    rows: Iterable[Mapping[str, str]],
    key_fields: Tuple[str, ...],
    name: str,
    errors: List[str],
) -> Dict[Tuple[str, ...], Dict[str, str]]:
    index: Dict[Tuple[str, ...], Dict[str, str]] = {}
    for row in rows:
        key = tuple(str(row.get(field) or "") for field in key_fields)
        if not all(key):
            errors.append(f"{name}: row has blank key {key_fields}: {row}")
            continue
        prior = index.get(key)
        normalized = dict(row)
        if prior is None:
            index[key] = normalized
        elif prior != normalized:
            errors.append(f"{name}: conflicting duplicate key {key!r}")
    return index


def preference_value(primary: Mapping[str, str], fallback: Mapping[str, str], field: str) -> str:
    return str(primary.get(field) or fallback.get(field) or "")


def category_family(category: str) -> str:
    return CATEGORY_FAMILIES.get(category, "")


def default_scope(declaration_kind: str) -> str:
    if declaration_kind in {"local", "parameter"}:
        return "local-parameter-cast-history-scope"
    if declaration_kind:
        return "outside-local-parameter-cast-history-scope"
    return "unknown-declaration-scope"


def coverage_status(
    declaration_kind: str,
    reconciliation: Optional[Mapping[str, str]],
) -> str:
    if reconciliation:
        status = str(reconciliation.get("classification_status") or "").strip()
        if status:
            return status
        if str(reconciliation.get("reconciliation_status") or "") == "prevalence-only":
            return "prevalence-only-no-inventory"
        return "inventory-status-missing"
    if declaration_kind in {"local", "parameter"}:
        return "missing-reconciliation-record"
    return "not-cast-history-eligible"


def category_consistency(
    coverage: str,
    subject_fact_emitted: bool,
    subject: Optional[Mapping[str, str]],
    category: str,
) -> str:
    if coverage == "classified":
        if not subject:
            return "classified-but-paper-subject-missing"
        if not category:
            return "classified-but-semantic-category-missing"
        return "classified-category-present"
    if subject_fact_emitted and not subject:
        return "subject-fact-emitted-but-paper-subject-missing"
    if not subject_fact_emitted and subject:
        return "paper-subject-present-without-emitted-flag"
    if coverage == "observed-no-final-history":
        return "observed-without-final-history"
    if coverage == "declared-no-observed-facts":
        return "no-observed-subject-facts"
    if coverage == "not-cast-history-eligible":
        return "not-applicable"
    if coverage == "out-of-scope-unavailable-api-parameter":
        return "not-applicable-unavailable-api"
    return "not-classified"


def sort_key(row: Mapping[str, Any]) -> Tuple[str, str, int, int, str]:
    return (
        str(row.get("project") or ""),
        str(row.get("source_file") or ""),
        int(str(row.get("source_line") or "0") or 0),
        int(str(row.get("source_column") or "0") or 0),
        str(row.get("declaration_id") or ""),
    )


def make_row(
    key: Tuple[str, str],
    prevalence: Optional[Mapping[str, str]],
    reconciliation: Optional[Mapping[str, str]],
    subject: Optional[Mapping[str, str]],
) -> Dict[str, Any]:
    project, declaration_id = key
    declaration_kind = preference_value(prevalence or {}, reconciliation or {}, "declaration_kind")
    if not declaration_kind:
        declaration_kind = str((reconciliation or {}).get("prevalence_declaration_kind") or
                               (reconciliation or {}).get("inventory_subject_kind") or "")

    coverage = coverage_status(declaration_kind, reconciliation)
    subject_id = str((reconciliation or {}).get("subject_id") or "")
    subject_emitted = bool_value((reconciliation or {}).get("subject_fact_emitted"))
    has_history = bool_value((reconciliation or {}).get("has_final_history"))
    category = str((subject or {}).get("primary_category") or "")

    # Subject evidence is authoritative for the emitted population.  Preserve
    # the inventory flags separately so an audit can find any mismatch.
    row: Dict[str, Any] = {
        "project": project,
        "declaration_id": declaration_id,
        "direct_void_pointer_prevalence": bool(prevalence),
        "prevalence_file": str((prevalence or {}).get("metrics_file") or ""),
        "declaration_kind": declaration_kind,
        "declaration_name": str((prevalence or {}).get("name") or
                                  (reconciliation or {}).get("prevalence_name") or ""),
        "declared_type": str((prevalence or {}).get("type") or
                              (reconciliation or {}).get("prevalence_type") or
                              (reconciliation or {}).get("inventory_declared_type") or ""),
        "declared_canonical_type": str((prevalence or {}).get("canonical_type") or
                                        (reconciliation or {}).get("prevalence_canonical_type") or
                                        (reconciliation or {}).get("inventory_canonical_type") or ""),
        "source_file": str((prevalence or {}).get("source_file") or
                            (reconciliation or {}).get("source_file") or ""),
        "source_line": integer_text((prevalence or {}).get("source_line") or
                                     (reconciliation or {}).get("source_line") or "0"),
        "source_column": integer_text((prevalence or {}).get("source_column") or
                                       (reconciliation or {}).get("source_column") or "0"),
        "reconciliation_status": str((reconciliation or {}).get("reconciliation_status") or
                                      ("prevalence-only" if prevalence else "inventory-only")),
        "subject_id": subject_id,
        "inventory_subject_kind": str((reconciliation or {}).get("inventory_subject_kind") or ""),
        "inventory_declared_type": str((reconciliation or {}).get("inventory_declared_type") or ""),
        "inventory_canonical_type": str((reconciliation or {}).get("inventory_canonical_type") or ""),
        "outer_pointer_kind": str((reconciliation or {}).get("outer_pointer_kind") or ""),
        "outer_void_pointer": bool_value((reconciliation or {}).get("outer_void_pointer")),
        "eligible_for_cast_history_classification": bool_value(
            (reconciliation or {}).get("eligible_for_cast_history_classification")),
        "semantic_classification_scope": str(
            (reconciliation or {}).get("semantic_classification_scope") or ""),
        "owner_definition_available": bool_value(
            (reconciliation or {}).get("owner_definition_available")),
        "eligibility_exclusion_reason": str(
            (reconciliation or {}).get("eligibility_exclusion_reason") or ""),
        "subject_fact_emitted": subject_emitted,
        "has_final_history": has_history,
        "classification_status": coverage,
        "classification_scope": str(
            (reconciliation or {}).get("semantic_classification_scope") or
            default_scope(declaration_kind)),
        "semantic_category": category,
        "semantic_family": category_family(category),
        "category_source": "paper-subject-facts" if subject else "",
        "category_consistency": category_consistency(coverage, subject_emitted, subject, category),
        "canonical_history_id": str((subject or {}).get("canonical_history_id") or ""),
        "history_shape": str((subject or {}).get("history_shape") or ""),
        "history_risk": str((subject or {}).get("history_risk") or ""),
        "behavior": str((subject or {}).get("behavior") or ""),
        "semantic_mediation_kind": str((subject or {}).get("semantic_mediation_kind") or ""),
        "semantic_mediation_family": str((subject or {}).get("semantic_mediation_family") or ""),
        "callback_mediation_state": str((subject or {}).get("callback_mediation_state") or ""),
        "transport_only": bool_value((subject or {}).get("transport_only")),
        "deref_read": bool_value((subject or {}).get("deref_read")),
        "deref_write_or_readwrite": bool_value((subject or {}).get("deref_write_or_readwrite")),
        "pointer_arithmetic": bool_value((subject or {}).get("pointer_arithmetic")),
        "array_index": bool_value((subject or {}).get("array_index")),
        "field_mediated_active": bool_value((subject or {}).get("field_mediated_active")),
        "callback_mediated_active": bool_value((subject or {}).get("callback_mediated_active")),
        "return_alias_mediated_active": bool_value((subject or {}).get("return_alias_mediated_active")),
        "external_escape": bool_value((subject or {}).get("external_escape")),
        "active_concrete_types": str((subject or {}).get("active_concrete_types") or ""),
        "active_byte_types": str((subject or {}).get("active_byte_types") or ""),
        "active_write_concrete_types": str((subject or {}).get("active_write_concrete_types") or ""),
        "active_guards": str((subject or {}).get("active_guards") or ""),
        "operation_kinds": str((subject or {}).get("operation_kinds") or ""),
        "paper_facts_file": str((subject or {}).get("paper_facts_file") or ""),
    }
    return row


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            rendered: Dict[str, Any] = {}
            for field in fields:
                value = row.get(field, "")
                rendered[field] = bool_text(value) if isinstance(value, bool) else value
            writer.writerow(rendered)


def write_jsonl(path: Path, rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")


def summary_rows(rows: Iterable[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    counters: Dict[str, Counter[str]] = {}
    for row in rows:
        project = str(row.get("project") or "")
        counter = counters.setdefault(project, Counter())
        counter["direct_void_pointer_declarations"] += int(bool(row.get("direct_void_pointer_prevalence")))
        kind = str(row.get("declaration_kind") or "")
        if kind in {"global", "field", "return", "local", "parameter"}:
            counter[kind + "s" if kind != "global" else "globals"] += 1
        status = str(row.get("classification_status") or "")
        status_columns = {
            "not-cast-history-eligible": "not_cast_history_eligible",
            "out-of-scope-unavailable-api-parameter":
                "out_of_scope_unavailable_api_parameter",
            "classified": "classified",
            "observed-no-final-history": "observed_no_final_history",
            "declared-no-observed-facts": "declared_no_observed_facts",
            "final-history-not-emitted": "final_history_not_emitted",
            "prevalence-only-no-inventory": "prevalence_only_no_inventory",
            "missing-reconciliation-record": "prevalence_only_no_inventory",
        }
        if status in status_columns:
            counter[status_columns[status]] += 1
        if str(row.get("reconciliation_status") or "") == "inventory-only":
            counter["inventory_only"] += 1
        category = str(row.get("semantic_category") or "")
        if category:
            counter["rows_with_semantic_category"] += 1
            counter[category] += 1

    result: List[Dict[str, Any]] = []
    for project in sorted(counters):
        row: Dict[str, Any] = {"project": project}
        for field in SUMMARY_FIELDS[1:]:
            row[field] = counters[project][field]
        result.append(row)
    return result


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prevalence-rows", type=Path, required=True,
                        help="aggregate_cast_prevalence_metrics.py pointer_declaration_rows.csv")
    parser.add_argument("--reconciliation-rows", type=Path, required=True,
                        help="aggregate_void_pointer_reconciliation.py void_pointer_reconciliation_rows.csv")
    parser.add_argument("--paper-subject-facts", type=Path, required=True,
                        help="aggregate_paper_facts.py paper_subject_facts.csv")
    parser.add_argument("--out", type=Path, required=True,
                        help="Output path for void_pointer_classification_rows.csv")
    parser.add_argument("--strict", action="store_true",
                        help="Fail on incompatible joins or classified rows without semantic categories")
    args = parser.parse_args(argv)

    errors: List[str] = []
    try:
        prevalence_rows = read_csv(args.prevalence_rows.resolve(), {
            "project", "id", "source_file", "source_line", "source_column",
            "declaration_kind", "name", "type", "canonical_type", "is_void_pointer",
        })
        reconciliation_rows = read_csv(args.reconciliation_rows.resolve(), {
            "project", "declaration_id", "reconciliation_status", "subject_id",
            "classification_status", "subject_fact_emitted", "has_final_history",
        })
        paper_rows = read_csv(args.paper_subject_facts.resolve(), {
            "project", "subject_id", "primary_category", "has_final_history",
            "outer_void_pointer", "subject_kind",
        })
    except InputError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    direct_prevalence = [row for row in prevalence_rows if bool_value(row.get("is_void_pointer"))]
    prevalence_index = unique_index(direct_prevalence, ("project", "id"), "prevalence", errors)
    reconciliation_index = unique_index(reconciliation_rows, ("project", "declaration_id"), "reconciliation", errors)
    paper_index = unique_index(paper_rows, ("project", "subject_id"), "paper-subject-facts", errors)

    all_keys = sorted(set(prevalence_index) | set(reconciliation_index))
    rows: List[Dict[str, Any]] = []
    for key in all_keys:
        reconciliation = reconciliation_index.get(key)
        subject_id = str((reconciliation or {}).get("subject_id") or "")
        subject = paper_index.get((key[0], subject_id)) if subject_id else None
        row = make_row(key, prevalence_index.get(key), reconciliation, subject)
        rows.append(row)

        # These invariant violations usually indicate incompatible aggregate
        # inputs rather than a meaningful semantic state.
        if row["classification_status"] == "classified" and not row["semantic_category"]:
            errors.append(
                f"{key}: classified declaration lacks a paper-facts semantic category")
        if row["subject_fact_emitted"] and not subject:
            errors.append(
                f"{key}: inventory says a subject fact was emitted but paper_subject_facts has no "
                f"row for {subject_id!r}")
        if subject and not bool_value(subject.get("outer_void_pointer")):
            errors.append(f"{key}: joined paper subject is not an outer void-data-pointer")
        if subject and str(subject.get("subject_kind") or "") not in {"local", "parameter"}:
            errors.append(f"{key}: joined paper subject is not local/parameter")

    rows.sort(key=sort_key)
    output = args.out.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    write_csv(output, ROW_FIELDS, rows)
    write_jsonl(output.with_suffix(".jsonl"), rows)
    summaries = summary_rows(rows)
    summary_path = output.with_name(output.stem + "_by_project.csv")
    write_csv(summary_path, SUMMARY_FIELDS, summaries)
    api_rows = [
        row for row in rows
        if row["classification_status"] ==
        "out-of-scope-unavailable-api-parameter"
    ]
    api_path = output.with_name("void_pointer_api_interface_rows.csv")
    write_csv(api_path, ROW_FIELDS, api_rows)
    write_jsonl(api_path.with_suffix(".jsonl"), api_rows)

    manifest = {
        "schemaName": "cast-history-void-pointer-classification-rows-manifest",
        "schemaVersion": 1,
        "toolVersion": VERSION,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "inputs": {
            "prevalenceRows": args.prevalence_rows.resolve().as_posix(),
            "reconciliationRows": args.reconciliation_rows.resolve().as_posix(),
            "paperSubjectFacts": args.paper_subject_facts.resolve().as_posix(),
        },
        "outputs": {
            "rows": output.as_posix(),
            "jsonl": output.with_suffix(".jsonl").as_posix(),
            "byProject": summary_path.as_posix(),
            "apiInterfaceRows": api_path.as_posix(),
            "apiInterfaceJsonl": api_path.with_suffix(".jsonl").as_posix(),
        },
        "rows": len(rows),
        "apiInterfaceRows": len(api_rows),
        "directVoidPointerPrevalenceRows": sum(bool(row["direct_void_pointer_prevalence"]) for row in rows),
        "rowsWithSemanticCategory": sum(bool(row["semantic_category"]) for row in rows),
        "coverageStatusCounts": dict(sorted(Counter(str(row["classification_status"]) for row in rows).items())),
        "categoryCounts": dict(sorted(Counter(str(row["semantic_category"]) for row in rows if row["semantic_category"]).items())),
        "warningsOrErrors": errors,
    }
    manifest_path = output.with_name(output.stem + ".manifest.json")
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for error in errors:
        print(f"warning: {error}", file=sys.stderr)
    if args.strict and errors:
        return 1
    print(f"wrote {len(rows)} void-pointer classification rows to {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

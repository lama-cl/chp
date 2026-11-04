#!/usr/bin/env python3
"""Reconcile direct ``void *`` prevalence with paper-facts declaration inventory.

C6g5f4d introduces two deliberately separate reporting populations:

* prevalence sidecars count project-owned *direct outer* pointer declarations;
* paper facts inventory every project-owned local/parameter direct pointer
  declaration, including values that never acquire a cast-history observation.

This program joins those records by the stable ``pointer-decl:...`` identifier
and emits both aggregate and row-level audit tables. Nested ``void *`` positions
inside function-pointer types are included as a separate count; they are not
silently added to the historical direct-declaration prevalence denominator.
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, MutableMapping, Optional, Sequence, Tuple

PAPER_SCHEMA_NAME = "cast-history-tu-paper-facts"
PAPER_SCHEMA_VERSIONS = {4, 5, 6, 7, 8, 9}
PREVALENCE_SCHEMA_NAME = "cast-history-prevalence-metrics"
PREVALENCE_SCHEMA_VERSIONS = {1, 2}
VERSION = "v5-c6g5f4j-void-pointer-reconciliation"

SUMMARY_FIELDS = (
    "project",
    "prevalence_files",
    "paper_facts_files",
    "project_scope_complete",
    "direct_void_pointer_declarations",
    "direct_void_pointer_globals",
    "direct_void_pointer_fields",
    "direct_void_pointer_returns",
    "direct_void_pointer_locals",
    "direct_void_pointer_parameters",
    "direct_void_pointer_locals_parameters",
    "nested_function_type_void_pointer_positions",
    "inventory_void_pointer_locals_parameters",
    "eligible_void_pointer_locals_parameters",
    "excluded_unavailable_api_void_pointer_parameters",
    "subject_fact_emitted",
    "with_final_history",
    "classified",
    "observed_no_final_history",
    "declared_no_observed_facts",
    "final_history_not_emitted",
    "prevalence_only_rows",
    "inventory_only_rows",
)

ROW_FIELDS = (
    "project",
    "declaration_id",
    "reconciliation_status",
    "prevalence_file",
    "prevalence_declaration_kind",
    "prevalence_name",
    "prevalence_type",
    "prevalence_canonical_type",
    "source_file",
    "source_line",
    "source_column",
    "paper_facts_file",
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
)


def bool_text(value: Any) -> str:
    return "yes" if value is True else "no" if value is False else ""


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
            normalized = {}
            for field in fields:
                value = row.get(field, "")
                normalized[field] = bool_text(value) if isinstance(value, bool) else value
            writer.writerow(normalized)


def source_value(row: Mapping[str, Any], field: str, default: Any = "") -> Any:
    source = row.get("source")
    return source.get(field, default) if isinstance(source, Mapping) else default


def discover(root: Path, suffix: str) -> List[Path]:
    return sorted(path for path in root.rglob(f"*{suffix}") if path.is_file())


def path_is_partial(path: Path) -> bool:
    return "partial-paper-facts" in path.parts or "partial-prevalence" in path.parts


def load_documents(
    paths: Iterable[Path], expected_schema: str, allowed_versions: set[int],
    errors: List[str], include_partial: bool,
) -> List[Tuple[Path, Mapping[str, Any]]]:
    result: List[Tuple[Path, Mapping[str, Any]]] = []
    for path in paths:
        if not include_partial and path_is_partial(path):
            continue
        try:
            with path.open(encoding="utf-8",errors='replace') as handle:
                data = json.load(handle)
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: cannot read JSON: {exc}")
            continue
        if not isinstance(data, Mapping):
            errors.append(f"{path}: top-level JSON must be an object")
            continue
        if data.get("schemaName") != expected_schema:
            errors.append(f"{path}: unexpected schemaName {data.get('schemaName')!r}")
            continue
        version = data.get("schemaVersion")
        if version not in allowed_versions:
            errors.append(f"{path}: unsupported schemaVersion {version!r}")
            continue
        result.append((path, data))
    return result


def infer_project_name(data: Mapping[str, Any], path: Path, root: Path) -> str:
    project = data.get("project")
    if isinstance(project, Mapping):
        identifier = str(project.get("id") or "")
        if identifier:
            return identifier
        declared_root = str(project.get("root") or "")
        if declared_root:
            return Path(declared_root).name or declared_root
    try:
        relative = path.relative_to(root)
        return relative.parts[0] if relative.parts else path.parent.name
    except ValueError:
        return path.parent.name


def paper_project_name(data: Mapping[str, Any], path: Path, root: Path) -> str:
    # Paper facts deliberately omit project metadata.  Reuse the common output
    # layout convention: <root>/<project>/**/file.cast-paper-facts.json.
    try:
        relative = path.relative_to(root)
        if relative.parts:
            return relative.parts[0]
    except ValueError:
        pass
    return path.parent.name


def merge_unique(
    target: MutableMapping[str, Tuple[Mapping[str, Any], Path]], record: Mapping[str, Any],
    path: Path, kind: str, errors: List[str],
) -> None:
    record_id = str(record.get("id") or record.get("declarationId") or "")
    if not record_id:
        errors.append(f"{path}: {kind} has no stable id")
        return
    previous = target.get(record_id)
    if previous is None:
        target[record_id] = (record, path)
    elif previous[0] != record:
        errors.append(f"{path}: conflicting duplicate {kind} {record_id!r}")


def is_direct_void_pointer(row: Mapping[str, Any]) -> bool:
    return row.get("isVoidPointer") is True


def inventory_is_void_local_or_parameter(row: Mapping[str, Any]) -> bool:
    declared = row.get("declaredType")
    return (
        row.get("subjectKind") in {"local", "parameter"}
        and isinstance(declared, Mapping)
        and declared.get("outerPointerKind") == "void-data-pointer"
    )


def to_row(
    project: str,
    declaration_id: str,
    prevalence: Optional[Tuple[Mapping[str, Any], Path]],
    inventory: Optional[Tuple[Mapping[str, Any], Path]],
) -> Dict[str, Any]:
    if prevalence and inventory:
        status = "matched"
    elif prevalence:
        status = "prevalence-only"
    else:
        status = "inventory-only"

    row: Dict[str, Any] = {
        "project": project,
        "declaration_id": declaration_id,
        "reconciliation_status": status,
    }
    if prevalence:
        value, path = prevalence
        row.update({
            "prevalence_file": path.as_posix(),
            "prevalence_declaration_kind": value.get("declarationKind", ""),
            "prevalence_name": value.get("name", ""),
            "prevalence_type": value.get("type", ""),
            "prevalence_canonical_type": value.get("canonicalType", ""),
            "source_file": source_value(value, "file"),
            "source_line": source_value(value, "line"),
            "source_column": source_value(value, "column"),
        })
    if inventory:
        value, path = inventory
        declared = value.get("declaredType")
        declared = declared if isinstance(declared, Mapping) else {}
        # Inventory source is authoritative when prevalence is absent.
        row.setdefault("source_file", source_value(value, "file"))
        row.setdefault("source_line", source_value(value, "line"))
        row.setdefault("source_column", source_value(value, "column"))
        if not row.get("source_file"):
            row["source_file"] = source_value(value, "file")
            row["source_line"] = source_value(value, "line")
            row["source_column"] = source_value(value, "column")
        row.update({
            "paper_facts_file": path.as_posix(),
            "subject_id": value.get("subjectId", ""),
            "inventory_subject_kind": value.get("subjectKind", ""),
            "inventory_declared_type": declared.get("text", ""),
            "inventory_canonical_type": declared.get("canonicalText", ""),
            "outer_pointer_kind": declared.get("outerPointerKind", ""),
            "outer_void_pointer": declared.get("outerVoidPointer", False),
            "eligible_for_cast_history_classification": value.get(
                "eligibleForCastHistoryClassification", False),
            "semantic_classification_scope": value.get(
                "semanticClassificationScope", ""),
            "owner_definition_available": value.get(
                "ownerDefinitionAvailable", False),
            "eligibility_exclusion_reason": value.get(
                "eligibilityExclusionReason", ""),
            "subject_fact_emitted": value.get("subjectFactEmitted", False),
            "has_final_history": value.get("hasFinalHistory", False),
            "classification_status": value.get("classificationStatus", ""),
        })
    return row


def write_latex(summary_rows: Sequence[Mapping[str, Any]], caption: str, label: str) -> str:
    lines = [
        r"\begin{table*}[t]", r"\centering",
        f"\\caption{{{latex_escape(caption)}}}",
        f"\\label{{{latex_escape(label)}}}",
        r"\begin{tabular}{lrrrrrrrrrr}", r"\toprule",
        r"Project & Direct \voidp & Globals & Fields & Returns & Local/param & API-only & Eligible & Classified & No facts & Nested fn-type \\",
        r"\midrule",
    ]
    for row in summary_rows:
        lines.append(
            f"{latex_escape(row['project'])} & {row['direct_void_pointer_declarations']} & "
            f"{row['direct_void_pointer_globals']} & {row['direct_void_pointer_fields']} & "
            f"{row['direct_void_pointer_returns']} & {row['direct_void_pointer_locals_parameters']} & "
            f"{row['excluded_unavailable_api_void_pointer_parameters']} & "
            f"{row['eligible_void_pointer_locals_parameters']} & {row['classified']} & "
            f"{row['declared_no_observed_facts']} & "
            f"{row['nested_function_type_void_pointer_positions']} \\")
    total = {field: sum(int(row[field]) for row in summary_rows) for field in SUMMARY_FIELDS if field not in {"project", "prevalence_files", "paper_facts_files", "project_scope_complete"}}
    lines.extend([
        r"\midrule",
        f"Total & {total['direct_void_pointer_declarations']} & {total['direct_void_pointer_globals']} & "
        f"{total['direct_void_pointer_fields']} & {total['direct_void_pointer_returns']} & "
        f"{total['direct_void_pointer_locals_parameters']} & "
        f"{total['excluded_unavailable_api_void_pointer_parameters']} & "
        f"{total['eligible_void_pointer_locals_parameters']} & {total['classified']} & "
        f"{total['declared_no_observed_facts']} & "
        f"{total['nested_function_type_void_pointer_positions']} \\",
        r"\bottomrule", r"\end{tabular}", r"\end{table*}", "",
    ])
    return "\n".join(lines)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prevalence-root", type=Path, required=True,
                        help="Root recursively containing *.cast-prevalence.json sidecars")
    parser.add_argument("--paper-facts-root", type=Path, required=True,
                        help="Root recursively containing *.cast-paper-facts.json sidecars")
    parser.add_argument("--out-dir", type=Path, required=True,
                        help="Directory for reconciliation CSV/JSON/LaTeX outputs")
    parser.add_argument("--caption", default=(
        "Direct \\voidp prevalence and local/parameter cast-history coverage. "
        "Nested function-type positions are reported separately."))
    parser.add_argument("--label", default="tab:void-pointer-reconciliation")
    parser.add_argument("--include-partial", action="store_true",
                        help="Include partial sidecars; disabled by default to preserve completed-only coverage")
    parser.add_argument("--strict", action="store_true",
                        help="Treat malformed sidecars and reconciliation mismatches as fatal")
    args = parser.parse_args(argv)

    prevalence_root = args.prevalence_root.resolve()
    paper_root = args.paper_facts_root.resolve()
    out_dir = args.out_dir.resolve()
    errors: List[str] = []

    prevalence_paths = discover(prevalence_root, ".cast-prevalence.json")
    paper_paths = discover(paper_root, ".cast-paper-facts.json")
    if not prevalence_paths:
        print(f"error: no *.cast-prevalence.json files under {prevalence_root}", file=sys.stderr)
        return 2
    if not paper_paths:
        print(f"error: no *.cast-paper-facts.json files under {paper_root}", file=sys.stderr)
        return 2

    prevalence_documents = load_documents(
        prevalence_paths, PREVALENCE_SCHEMA_NAME, PREVALENCE_SCHEMA_VERSIONS,
        errors, args.include_partial)
    paper_documents = load_documents(
        paper_paths, PAPER_SCHEMA_NAME, PAPER_SCHEMA_VERSIONS,
        errors, args.include_partial)
    if not paper_documents:
        print("error: no schema-version-4-or-later paper-facts sidecars available for reconciliation", file=sys.stderr)
        for item in errors:
            print(f"warning: {item}", file=sys.stderr)
        return 2

    prevalence: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    nested_positions: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    inventory: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    prevalence_file_counts: Dict[str, int] = defaultdict(int)
    paper_file_counts: Dict[str, int] = defaultdict(int)
    scope_complete: Dict[str, bool] = defaultdict(lambda: True)

    for path, data in prevalence_documents:
        project = infer_project_name(data, path, prevalence_root)
        prevalence_file_counts[project] += 1
        records = data.get("pointerDeclarations")
        if not isinstance(records, list):
            errors.append(f"{path}: pointerDeclarations must be an array")
            continue
        for record in records:
            if not isinstance(record, Mapping) or not is_direct_void_pointer(record):
                continue
            merge_unique(prevalence[project], record, path, "direct void-pointer declaration", errors)
        positions = data.get("functionTypeVoidPointerPositions", [])
        if not isinstance(positions, list):
            errors.append(f"{path}: functionTypeVoidPointerPositions must be an array when present")
            continue
        for position in positions:
            if not isinstance(position, Mapping):
                errors.append(f"{path}: non-object function-type void-pointer position")
                continue
            merge_unique(nested_positions[project], position, path,
                         "nested function-type void-pointer position", errors)

    for path, data in paper_documents:
        project = paper_project_name(data, path, paper_root)
        paper_file_counts[project] += 1
        coverage = data.get("coverage")
        if not isinstance(coverage, Mapping):
            errors.append(f"{path}: coverage must be an object")
            scope_complete[project] = False
        elif coverage.get("declaredPointerInventoryScopeComplete") is not True:
            scope_complete[project] = False
        records = data.get("declaredPointerInventory")
        if not isinstance(records, list):
            errors.append(f"{path}: declaredPointerInventory must be an array")
            scope_complete[project] = False
            continue
        for record in records:
            if not isinstance(record, Mapping) or not inventory_is_void_local_or_parameter(record):
                continue
            merge_unique(inventory[project], record, path,
                         "void-pointer local/parameter inventory row", errors)

    projects = sorted(set(prevalence) | set(inventory) | set(nested_positions) |
                      set(prevalence_file_counts) | set(paper_file_counts))
    summary_rows: List[Dict[str, Any]] = []
    audit_rows: List[Dict[str, Any]] = []
    for project in projects:
        direct = prevalence[project]
        declared = inventory[project]
        direct_by_kind: Dict[str, int] = defaultdict(int)
        for record, _ in direct.values():
            direct_by_kind[str(record.get("declarationKind") or "other")] += 1
        local_param_ids = {
            record_id for record_id, (record, _) in direct.items()
            if record.get("declarationKind") in {"local", "parameter"}
        }
        inventory_ids = set(declared)
        rows: List[Dict[str, Any]] = []
        for declaration_id in sorted(local_param_ids | inventory_ids):
            current = to_row(project, declaration_id, direct.get(declaration_id),
                             declared.get(declaration_id))
            rows.append(current)
        audit_rows.extend(rows)

        inventory_rows = [record for record, _ in declared.values()]
        eligible_rows = [
            row for row in inventory_rows
            if row.get("eligibleForCastHistoryClassification") is True
        ]
        statuses = defaultdict(int)
        for row in eligible_rows:
            statuses[str(row.get("classificationStatus") or "")] += 1
        eligible = len(eligible_rows)
        excluded_unavailable_api = sum(
            str(row.get("classificationStatus") or "") ==
            "out-of-scope-unavailable-api-parameter"
            for row in inventory_rows)
        subject_facts = sum(row.get("subjectFactEmitted") is True
                            for row in eligible_rows)
        final_history = sum(row.get("hasFinalHistory") is True
                            for row in eligible_rows)
        summary_rows.append({
            "project": project,
            "prevalence_files": prevalence_file_counts[project],
            "paper_facts_files": paper_file_counts[project],
            "project_scope_complete": bool(scope_complete[project]),
            "direct_void_pointer_declarations": len(direct),
            "direct_void_pointer_globals": direct_by_kind["global"],
            "direct_void_pointer_fields": direct_by_kind["field"],
            "direct_void_pointer_returns": direct_by_kind["return"],
            "direct_void_pointer_locals": direct_by_kind["local"],
            "direct_void_pointer_parameters": direct_by_kind["parameter"],
            "direct_void_pointer_locals_parameters": len(local_param_ids),
            "nested_function_type_void_pointer_positions": len(nested_positions[project]),
            "inventory_void_pointer_locals_parameters": len(inventory_rows),
            "eligible_void_pointer_locals_parameters": eligible,
            "excluded_unavailable_api_void_pointer_parameters":
                excluded_unavailable_api,
            "subject_fact_emitted": subject_facts,
            "with_final_history": final_history,
            "classified": statuses["classified"],
            "observed_no_final_history": statuses["observed-no-final-history"],
            "declared_no_observed_facts": statuses["declared-no-observed-facts"],
            "final_history_not_emitted": statuses["final-history-not-emitted"],
            "prevalence_only_rows": sum(
                row["reconciliation_status"] == "prevalence-only" for row in rows),
            "inventory_only_rows": sum(
                row["reconciliation_status"] == "inventory-only" for row in rows),
        })

    out_dir.mkdir(parents=True, exist_ok=True)
    write_csv(out_dir / "void_pointer_reconciliation_by_project.csv", SUMMARY_FIELDS, summary_rows)
    write_csv(out_dir / "void_pointer_reconciliation_rows.csv", ROW_FIELDS, audit_rows)
    (out_dir / "void_pointer_reconciliation_by_project.json").write_text(
        json.dumps(summary_rows, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (out_dir / "void_pointer_reconciliation.tex").write_text(
        write_latex(summary_rows, args.caption, args.label), encoding="utf-8")

    mismatch_count = sum(
        int(row["prevalence_only_rows"]) + int(row["inventory_only_rows"])
        for row in summary_rows)
    totals = {
        field: sum(int(row[field]) for row in summary_rows)
        for field in SUMMARY_FIELDS
        if field not in {"project", "prevalence_files", "paper_facts_files", "project_scope_complete"}
    }
    manifest = {
        "schemaName": "cast-history-void-pointer-reconciliation-manifest",
        "schemaVersion": 2,
        "toolVersion": VERSION,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "prevalenceRoot": prevalence_root.as_posix(),
        "paperFactsRoot": paper_root.as_posix(),
        "includePartial": args.include_partial,
        "projects": [row["project"] for row in summary_rows],
        "totals": totals,
        "allProjectScopesComplete": all(bool(row["project_scope_complete"]) for row in summary_rows),
        "reconciliationMismatchRows": mismatch_count,
        "warningsOrErrors": errors,
    }
    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for item in errors:
        print(f"warning: {item}", file=sys.stderr)
    if args.strict and (errors or mismatch_count):
        return 1
    print(f"reconciled {len(prevalence_documents)} prevalence and {len(paper_documents)} paper-facts sidecars across {len(summary_rows)} project(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

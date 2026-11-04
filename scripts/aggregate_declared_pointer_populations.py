#!/usr/bin/env python3
"""Aggregate C6g5f4 declared-pointer populations from paper-facts sidecars.

This script computes two cast-history-only counts for each fixed semantic
category c = primaryCategory:

  V_c^CH: final local/parameter subjects declared as void-data-pointer
  P_c^CH: final local/parameter subjects declared as nonvoid-data-pointer

The input field `declaredType.outerPointerKind` is emitted from Clang QualType
in schema-version-2 and schema-version-3 paper-facts sidecars. This script deliberately does not
infer P_c^CH from printable type text in old schema-version-1 sidecars.
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, MutableMapping, Optional, Sequence, Tuple

SCHEMA_NAME = "cast-history-tu-paper-facts"
SUPPORTED_SCHEMA_VERSIONS = {1, 2, 3, 4, 5, 6, 7, 8, 9}
OUTER_POINTER_KINDS = {
    "void-data-pointer",
    "nonvoid-data-pointer",
    "function-pointer",
    "not-pointer",
    "unknown",
}
PRIMARY_CATEGORIES = (
    "OpaqueTransportOrViewOnly",
    "MonomorphicActiveSpecialization",
    "ByteOrientedActiveAccess",
    "GuardedActiveReinterpretation",
    "UnguardedMultiTargetActiveReinterpretation",
    "MixedOrProvenanceSensitive",
    "ExternalUnknown",
)
SEMANTIC_FAMILY_BY_CATEGORY = {
    "OpaqueTransportOrViewOnly": "Structured / view-only",
    "MonomorphicActiveSpecialization": "Structured / bounded",
    "ByteOrientedActiveAccess": "Translation-sensitive",
    "GuardedActiveReinterpretation": "Translation-sensitive",
    "UnguardedMultiTargetActiveReinterpretation": "Semantically wild / high-risk",
    "MixedOrProvenanceSensitive": "Semantically wild / high-risk",
    "ExternalUnknown": "External unknown",
}
AUDIT_FIELDS = (
    "project", "paper_facts_file", "schema_version", "subject_id", "entity",
    "subject_kind", "source_file", "source_line", "source_column",
    "function_qualified_name", "function_signature_key", "declared_type",
    "declared_canonical_type", "outer_void_pointer", "outer_pointer_kind",
    "outer_pointer_kind_known", "has_final_history", "primary_category",
    "semantic_family", "callback_mediation_state",
    "callback_is_callback", "callback_resolved_target",
    "callback_field_carrier", "callback_guarded_selection",
    "contributes_declared_void_pointer_params_locals",
    "contributes_declared_nonvoid_data_pointer_params_locals",
)
TABLE_FIELDS = (
    "project", "semantic_family", "primary_category",
    "declared_void_pointer_params_locals",
    "declared_nonvoid_data_pointer_params_locals",
)


def bool_value(value: Any) -> bool:
    return value is True or (isinstance(value, str) and value.strip().lower() in {"1", "true", "yes", "y"})


def bool_text(value: Any) -> str:
    return "yes" if value is True else "no" if value is False else ""


def as_obj(value: Any) -> Mapping[str, Any]:
    return value if isinstance(value, Mapping) else {}


def sorted_unique_strings(value: Any) -> List[str]:
    if isinstance(value, str):
        return [value] if value else []
    if not isinstance(value, (list, tuple, set)):
        return []
    return sorted({str(item) for item in value if str(item)})


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: bool_text(row.get(field)) if isinstance(row.get(field), bool) else row.get(field, "")
                             for field in fields})


def latex_escape(value: Any) -> str:
    replacements = {
        "\\": r"\textbackslash{}", "&": r"\&", "%": r"\%", "$": r"\$",
        "#": r"\#", "_": r"\_", "{": r"\{", "}": r"\}",
        "~": r"\textasciitilde{}", "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(ch, ch) for ch in str(value))


def discover(root: Path, include_partial: bool) -> List[Path]:
    paths: List[Path] = []
    for path in root.rglob("*.cast-paper-facts.json"):
        if not include_partial and "partial-paper-facts" in path.parts:
            continue
        paths.append(path)
    return sorted(paths)


def project_name(path: Path, root: Path, data: Mapping[str, Any]) -> str:
    """Match the project-directory inference used by aggregate_paper_facts.py."""
    try:
        parts = path.resolve().relative_to(root.resolve()).parts
    except ValueError:
        parts = path.parts
    markers = {
        "paper-facts", "c1-paper-facts", "reports", "summaries", "rounds",
        "import-round-01-paper-facts", "import-round-02-paper-facts",
    }
    for index, part in enumerate(parts):
        if part in markers and index > 0:
            return parts[index - 1]
    source = str(as_obj(data.get("tu")).get("sourceFile") or "")
    return Path(source).parent.name if source else (parts[0] if parts else path.parent.name)


def normalized_outer_pointer_kind(declared: Mapping[str, Any]) -> str:
    raw = str(declared.get("outerPointerKind") or "").strip()
    if raw in OUTER_POINTER_KINDS:
        return raw
    # Schema v1 contains an explicit boolean for this positive case only.
    if bool_value(declared.get("outerVoidPointer")):
        return "void-data-pointer"
    return ""


def flatten_subject(project: str, sidecar: Path, schema_version: int, subject: Mapping[str, Any]) -> Dict[str, Any]:
    declared = as_obj(subject.get("declaredType"))
    location = as_obj(subject.get("location"))
    function = as_obj(subject.get("function"))
    kind = normalized_outer_pointer_kind(declared)
    known = kind in {"void-data-pointer", "nonvoid-data-pointer", "function-pointer", "not-pointer"}
    final = bool_value(subject.get("hasFinalHistory"))
    callback_mediation = as_obj(subject.get("callbackMediation"))
    callback_flags = as_obj(callback_mediation.get("flags"))
    subject_kind = str(subject.get("subjectKind") or "")
    category = str(subject.get("primaryCategory") or "")
    eligible = final and subject_kind in {"local", "parameter"}
    return {
        "project": project,
        "paper_facts_file": sidecar.as_posix(),
        "schema_version": schema_version,
        "subject_id": str(subject.get("subjectId") or ""),
        "entity": str(subject.get("entity") or ""),
        "subject_kind": subject_kind,
        "source_file": str(location.get("file") or ""),
        "source_line": location.get("line", ""),
        "source_column": location.get("column", ""),
        "function_qualified_name": str(function.get("qualifiedName") or ""),
        "function_signature_key": str(function.get("signatureKey") or ""),
        "declared_type": str(declared.get("text") or ""),
        "declared_canonical_type": str(declared.get("canonicalText") or ""),
        "outer_void_pointer": bool_value(declared.get("outerVoidPointer")),
        "outer_pointer_kind": kind,
        "outer_pointer_kind_known": known,
        "has_final_history": final,
        "primary_category": category,
        "semantic_family": SEMANTIC_FAMILY_BY_CATEGORY.get(category, ""),
        "callback_mediation_state": str(callback_mediation.get("state") or ""),
        "callback_is_callback": bool_value(callback_flags.get("callback")),
        "callback_resolved_target": bool_value(callback_flags.get("resolvedTarget")),
        "callback_field_carrier": bool_value(callback_flags.get("fieldCarrier")),
        "callback_guarded_selection": bool_value(callback_flags.get("guardedSelection")),
        "contributes_declared_void_pointer_params_locals": eligible and kind == "void-data-pointer",
        "contributes_declared_nonvoid_data_pointer_params_locals": eligible and kind == "nonvoid-data-pointer",
    }


def merge_duplicate(existing: MutableMapping[str, Any], incoming: Mapping[str, Any], errors: List[str]) -> None:
    """Merge repeated header-origin rows only when their stable facts agree."""
    for field in ("entity", "subject_kind", "source_file", "source_line", "source_column"):
        left, right = existing.get(field), incoming.get(field)
        if left not in ("", None) and right not in ("", None) and left != right:
            errors.append(f"duplicate subject {existing.get('subject_id')!r} conflicts on {field}: {left!r} != {right!r}")
    for field in ("has_final_history", "contributes_declared_void_pointer_params_locals",
                  "contributes_declared_nonvoid_data_pointer_params_locals"):
        existing[field] = bool_value(existing.get(field)) or bool_value(incoming.get(field))
    old_kind, new_kind = str(existing.get("outer_pointer_kind") or ""), str(incoming.get("outer_pointer_kind") or "")
    if not old_kind and new_kind:
        existing["outer_pointer_kind"] = new_kind
        existing["outer_pointer_kind_known"] = bool_value(incoming.get("outer_pointer_kind_known"))
    elif old_kind and new_kind and old_kind != new_kind:
        errors.append(f"duplicate subject {existing.get('subject_id')!r} has conflicting outer pointer kinds: {old_kind!r} != {new_kind!r}")
    old_category, new_category = str(existing.get("primary_category") or ""), str(incoming.get("primary_category") or "")
    if old_category and new_category and old_category != new_category:
        errors.append(f"duplicate subject {existing.get('subject_id')!r} has conflicting categories: {old_category!r} != {new_category!r}")


def table_row(project: str, category: str, rows: Sequence[Mapping[str, Any]]) -> Dict[str, Any]:
    return {
        "project": project,
        "semantic_family": SEMANTIC_FAMILY_BY_CATEGORY.get(category, ""),
        "primary_category": category,
        "declared_void_pointer_params_locals": sum(bool_value(row.get("contributes_declared_void_pointer_params_locals")) for row in rows),
        "declared_nonvoid_data_pointer_params_locals": sum(bool_value(row.get("contributes_declared_nonvoid_data_pointer_params_locals")) for row in rows),
    }


def summarize(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    by_project_category: Dict[Tuple[str, str], List[Mapping[str, Any]]] = defaultdict(list)
    all_by_category: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    for row in rows:
        category = str(row.get("primary_category") or "")
        if category not in PRIMARY_CATEGORIES:
            continue
        project = str(row.get("project") or "")
        by_project_category[(project, category)].append(row)
        all_by_category[category].append(row)
    projects = sorted({project for project, _ in by_project_category})
    result: List[Dict[str, Any]] = []
    for project in projects:
        project_rows: List[Mapping[str, Any]] = []
        for category in PRIMARY_CATEGORIES:
            category_rows = by_project_category.get((project, category), [])
            project_rows.extend(category_rows)
            result.append(table_row(project, category, category_rows))
        total = table_row(project, "", project_rows)
        total["semantic_family"], total["primary_category"] = "Total", "All"
        result.append(total)
    if projects:
        all_rows: List[Mapping[str, Any]] = []
        for category in PRIMARY_CATEGORIES:
            category_rows = all_by_category.get(category, [])
            all_rows.extend(category_rows)
            result.append(table_row("Total", category, category_rows))
        total = table_row("Total", "", all_rows)
        total["semantic_family"], total["primary_category"] = "Total", "All"
        result.append(total)
    return result


def latex_table(rows: Sequence[Mapping[str, Any]]) -> str:
    lines = [
        r"\begin{table*}[t]", r"\centering",
        r"\caption{Final cast-history subjects declared as local/parameter pointers, grouped by semantic category. Declared \voidp counts $V_c^{CH}$; other declared data-pointer counts $P_c^{CH}$. These are cast-history corpus counts and are not restricted to externally aligned GenC2Rust subjects.}",
        r"\label{tab:declared-pointer-population}",
        r"\begin{tabular}{lllrr}", r"\toprule",
        r"Project & Semantic family & Category & Declared \voidp params/locals & Other declared data-pointer params/locals \\",
        r"\midrule",
    ]
    for row in rows:
        if row.get("project") == "Total" and row.get("primary_category") == "All":
            lines.append(r"\midrule")
        lines.append(
            f"{latex_escape(row.get('project', ''))} & "
            f"{latex_escape(row.get('semantic_family', ''))} & "
            f"{latex_escape(row.get('primary_category', ''))} & "
            f"{row.get('declared_void_pointer_params_locals', 0)} & "
            f"{row.get('declared_nonvoid_data_pointer_params_locals', 0)} \\\\")
    lines.extend([r"\bottomrule", r"\end{tabular}", r"\end{table}", ""])
    return "\n".join(lines)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Aggregate C6g5f4 declared local/parameter pointer populations.")
    parser.add_argument("--root", type=Path, required=True, help="Root recursively containing *.cast-paper-facts.json")
    parser.add_argument("--out-dir", type=Path, required=True, help="Directory for CSV, LaTeX, audit rows, and manifest")
    parser.add_argument("--project-name", default="", help="Override inferred project name for every input sidecar")
    parser.add_argument("--include-partial", action="store_true", help="Include partial-paper-facts sidecars (off by default because they are not C2-eligible final artifacts)")
    parser.add_argument("--strict", action="store_true", help="Treat malformed sidecars and duplicate conflicts as fatal")
    args = parser.parse_args(argv)

    root, out_dir = args.root.resolve(), args.out_dir.resolve()
    paths = discover(root, args.include_partial)
    if not paths:
        print(f"error: no eligible *.cast-paper-facts.json sidecars under {root}", file=sys.stderr)
        return 2
    out_dir.mkdir(parents=True, exist_ok=True)

    errors: List[str] = []
    subjects: Dict[Tuple[str, str], Dict[str, Any]] = {}
    schema_versions: Counter[str] = Counter()
    for path in paths:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: cannot parse JSON: {exc}")
            continue
        if not isinstance(data, Mapping):
            errors.append(f"{path}: top-level JSON must be an object")
            continue
        if data.get("schemaName") != SCHEMA_NAME:
            errors.append(f"{path}: unexpected schemaName {data.get('schemaName')!r}")
            continue
        version = data.get("schemaVersion")
        if version not in SUPPORTED_SCHEMA_VERSIONS:
            errors.append(f"{path}: unsupported schemaVersion {version!r}")
            continue
        schema_versions[str(version)] += 1
        project = args.project_name or project_name(path, root, data)
        raw_subjects = data.get("subjects")
        if not isinstance(raw_subjects, list):
            errors.append(f"{path}: subjects must be an array")
            continue
        for raw_subject in raw_subjects:
            if not isinstance(raw_subject, Mapping):
                errors.append(f"{path}: non-object subject row")
                continue
            row = flatten_subject(project, path, int(version), raw_subject)
            if not row["subject_id"]:
                errors.append(f"{path}: subject row lacks subjectId")
                continue
            key = (project, str(row["subject_id"]))
            prior = subjects.get(key)
            if prior is None:
                subjects[key] = row
            else:
                merge_duplicate(prior, row, errors)

    audit_rows = [subjects[key] for key in sorted(subjects)]
    table_rows = summarize(audit_rows)
    eligible = [row for row in audit_rows if bool_value(row.get("has_final_history")) and row.get("subject_kind") in {"local", "parameter"}]
    unavailable = sum(not bool_value(row.get("outer_pointer_kind_known")) for row in eligible)
    manifest = {
        "schemaName": "cast-history-declared-pointer-population-manifest",
        "schemaVersion": 1,
        "createdAt": datetime.now(timezone.utc).isoformat(),
        "root": root.as_posix(),
        "inputSidecars": [path.as_posix() for path in paths],
        "includePartial": args.include_partial,
        "sidecarsByPaperFactsSchemaVersion": dict(schema_versions),
        "subjectRows": len(audit_rows),
        "finalLocalParameterSubjects": len(eligible),
        "outerPointerKindCoverage": {
            "outerPointerKindUnavailable": unavailable,
            "complete": unavailable == 0,
        },
        "errors": errors,
    }
    write_csv(out_dir / "declared_pointer_subject_facts.csv", AUDIT_FIELDS, audit_rows)
    write_csv(out_dir / "declared_pointer_populations_by_category.csv", TABLE_FIELDS, table_rows)
    (out_dir / "declared_pointer_populations.tex").write_text(latex_table(table_rows), encoding="utf-8")
    (out_dir / "declared_pointer_populations_manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if unavailable:
        print(
            "warning: declared nonvoid data-pointer counts are incomplete: "
            f"{unavailable} final local/parameter subject(s) lack the C6g5f2/C6g5f4 AST-derived outerPointerKind field",
            file=sys.stderr)
    for error in errors:
        print(f"warning: {error}", file=sys.stderr)
    if args.strict and errors:
        return 1
    print(f"aggregated {len(paths)} sidecars into {len(audit_rows)} subject rows; wrote {len(table_rows)} category rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

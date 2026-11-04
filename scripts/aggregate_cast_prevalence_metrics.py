#!/usr/bin/env python3
"""Aggregate pointer/cast prevalence sidecars into paper-ready tables.

Schema v2 retains the original direct outer-pointer prevalence denominator and
also exports ``void *`` positions nested inside function-pointer types.  Those
nested positions are reported separately: they are not silently added to the
historical direct-declaration ``void_pointers`` count.
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

SCHEMA_NAME = "cast-history-prevalence-metrics"
SUPPORTED_SCHEMA_VERSIONS = {1, 2}
VERSION = "v2-c6g5f4d-prevalence-aggregation"

COUNT_FIELDS = (
    "pointers",
    "void_pointers",
    "function_pointers",
    "pointer_fields",
    "casts",
    "implicit_casts",
    "to_void_pointer_casts",
    "from_void_pointer_casts",
    "byte_pointer_casts",
    "return_pointer_declarations",
    "void_pointer_globals",
    "void_pointer_fields",
    "void_pointer_locals",
    "void_pointer_parameters",
    "void_pointer_returns",
    "function_type_void_pointer_positions",
)

SUMMARY_FIELDS = (
    "project",
) + COUNT_FIELDS + (
    "metric_files",
    "unique_pointer_declaration_rows",
    "unique_function_type_void_pointer_position_rows",
    "unique_cast_rows",
)

DECLARATION_FIELDS = (
    "project", "metrics_file", "id", "source_file", "source_line",
    "source_column", "declaration_kind", "name", "type", "canonical_type",
    "is_void_pointer", "is_function_pointer", "is_pointer_field",
)

FUNCTION_TYPE_POSITION_FIELDS = (
    "project", "metrics_file", "id", "owner_declaration_id",
    "source_file", "source_line", "source_column",
    "owner_declaration_kind", "owner_name", "position_kind",
    "position_path", "type", "canonical_type",
)

CAST_FIELDS = (
    "project", "metrics_file", "id", "source_file", "source_line",
    "source_column", "cast_kind", "is_implicit", "source_type",
    "source_canonical_type", "destination_type", "destination_canonical_type",
    "source_is_pointer", "destination_is_pointer", "to_void_pointer",
    "from_void_pointer", "byte_pointer_cast",
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
            normalized: Dict[str, Any] = {}
            for field in fields:
                value = row.get(field, "")
                normalized[field] = bool_text(value) if isinstance(value, bool) else value
            writer.writerow(normalized)


def write_jsonl(path: Path, rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")


def discover(root: Path) -> List[Path]:
    return sorted(path for path in root.rglob("*.cast-prevalence.json") if path.is_file())


def source_value(row: Mapping[str, Any], field: str, default: Any = "") -> Any:
    source = row.get("source")
    return source.get(field, default) if isinstance(source, Mapping) else default


def valid_document(path: Path, data: Mapping[str, Any]) -> List[str]:
    errors: List[str] = []
    if data.get("schemaName") != SCHEMA_NAME:
        errors.append(f"{path}: schemaName must be {SCHEMA_NAME!r}")
    version = data.get("schemaVersion")
    if version not in SUPPORTED_SCHEMA_VERSIONS:
        errors.append(f"{path}: schemaVersion must be one of {sorted(SUPPORTED_SCHEMA_VERSIONS)}")
    if not isinstance(data.get("project"), Mapping):
        errors.append(f"{path}: project object missing")
    if not isinstance(data.get("tu"), Mapping):
        errors.append(f"{path}: tu object missing")
    if not isinstance(data.get("pointerDeclarations"), list):
        errors.append(f"{path}: pointerDeclarations must be an array")
    if not isinstance(data.get("casts"), list):
        errors.append(f"{path}: casts must be an array")
    if version == 2 and not isinstance(data.get("functionTypeVoidPointerPositions"), list):
        errors.append(f"{path}: schema version 2 requires functionTypeVoidPointerPositions array")
    return errors


def project_name(data: Mapping[str, Any], path: Path, root: Path) -> str:
    project = data.get("project")
    if isinstance(project, Mapping) and str(project.get("id") or ""):
        return str(project["id"])
    if isinstance(project, Mapping) and str(project.get("root") or ""):
        return Path(str(project["root"])).name or str(project["root"])
    try:
        return path.relative_to(root).parts[0]
    except ValueError:
        return path.parent.name


def numeric_counts(
    declarations: Iterable[Mapping[str, Any]],
    casts: Iterable[Mapping[str, Any]],
    function_positions: Iterable[Mapping[str, Any]],
) -> Dict[str, int]:
    result = {field: 0 for field in COUNT_FIELDS}
    decls = list(declarations)
    cast_rows = list(casts)
    position_rows = list(function_positions)
    result["pointers"] = len(decls)
    result["void_pointers"] = sum(bool(row.get("isVoidPointer")) for row in decls)
    result["function_pointers"] = sum(bool(row.get("isFunctionPointer")) for row in decls)
    result["pointer_fields"] = sum(bool(row.get("isPointerField")) for row in decls)
    result["return_pointer_declarations"] = sum(
        row.get("declarationKind") == "return" for row in decls
    )
    for kind, field in (
        ("global", "void_pointer_globals"),
        ("field", "void_pointer_fields"),
        ("local", "void_pointer_locals"),
        ("parameter", "void_pointer_parameters"),
        ("return", "void_pointer_returns"),
    ):
        result[field] = sum(
            bool(row.get("isVoidPointer")) and row.get("declarationKind") == kind
            for row in decls
        )
    result["function_type_void_pointer_positions"] = len(position_rows)
    result["casts"] = sum(not bool(row.get("isImplicit")) for row in cast_rows)
    result["implicit_casts"] = sum(bool(row.get("isImplicit")) for row in cast_rows)
    result["to_void_pointer_casts"] = sum(bool(row.get("toVoidPointer")) for row in cast_rows)
    result["from_void_pointer_casts"] = sum(bool(row.get("fromVoidPointer")) for row in cast_rows)
    result["byte_pointer_casts"] = sum(bool(row.get("bytePointerCast")) for row in cast_rows)
    return result


def to_declaration_row(project: str, metrics_file: Path, record: Mapping[str, Any]) -> Dict[str, Any]:
    return {
        "project": project, "metrics_file": metrics_file.as_posix(),
        "id": record.get("id", ""), "source_file": source_value(record, "file"),
        "source_line": source_value(record, "line"), "source_column": source_value(record, "column"),
        "declaration_kind": record.get("declarationKind", ""), "name": record.get("name", ""),
        "type": record.get("type", ""), "canonical_type": record.get("canonicalType", ""),
        "is_void_pointer": record.get("isVoidPointer", False),
        "is_function_pointer": record.get("isFunctionPointer", False),
        "is_pointer_field": record.get("isPointerField", False),
    }


def to_function_type_position_row(project: str, metrics_file: Path, record: Mapping[str, Any]) -> Dict[str, Any]:
    return {
        "project": project, "metrics_file": metrics_file.as_posix(),
        "id": record.get("id", ""),
        "owner_declaration_id": record.get("ownerDeclarationId", ""),
        "source_file": source_value(record, "file"),
        "source_line": source_value(record, "line"),
        "source_column": source_value(record, "column"),
        "owner_declaration_kind": record.get("ownerDeclarationKind", ""),
        "owner_name": record.get("ownerName", ""),
        "position_kind": record.get("positionKind", ""),
        "position_path": record.get("positionPath", ""),
        "type": record.get("type", ""),
        "canonical_type": record.get("canonicalType", ""),
    }


def to_cast_row(project: str, metrics_file: Path, record: Mapping[str, Any]) -> Dict[str, Any]:
    return {
        "project": project, "metrics_file": metrics_file.as_posix(),
        "id": record.get("id", ""), "source_file": source_value(record, "file"),
        "source_line": source_value(record, "line"), "source_column": source_value(record, "column"),
        "cast_kind": record.get("castKind", ""), "is_implicit": record.get("isImplicit", False),
        "source_type": record.get("sourceType", ""),
        "source_canonical_type": record.get("sourceCanonicalType", ""),
        "destination_type": record.get("destinationType", ""),
        "destination_canonical_type": record.get("destinationCanonicalType", ""),
        "source_is_pointer": record.get("sourceIsPointer", False),
        "destination_is_pointer": record.get("destinationIsPointer", False),
        "to_void_pointer": record.get("toVoidPointer", False),
        "from_void_pointer": record.get("fromVoidPointer", False),
        "byte_pointer_cast": record.get("bytePointerCast", False),
    }


def merge_unique(
    target: MutableMapping[str, Tuple[Mapping[str, Any], Path]],
    record: Mapping[str, Any], path: Path, kind: str, errors: List[str],
) -> None:
    record_id = str(record.get("id") or "")
    if not record_id:
        errors.append(f"{path}: {kind} record has no id")
        return
    previous = target.get(record_id)
    if previous is None:
        target[record_id] = (record, path)
    elif previous[0] != record:
        errors.append(f"{path}: conflicting duplicate {kind} id {record_id!r}")


def render_latex(summary_rows: Sequence[Mapping[str, Any]], caption: str, label: str) -> str:
    header = (
        "Project & Pointers & \\voidp & Fn ptrs & Ptr fields & Casts & "
        "Implicit casts & To \\voidp & From \\voidp & Byte casts \\\\"
    )
    lines = [
        r"\begin{table}[t]", r"\centering", f"\\caption{{{latex_escape(caption)}}}",
        f"\\label{{{latex_escape(label)}}}", r"\begin{tabular}{lrrrrrrrrr}",
        r"\toprule", header, r"\midrule",
    ]
    for row in summary_rows:
        lines.append(
            f"{latex_escape(row['project'])} & {row['pointers']} & {row['void_pointers']} & "
            f"{row['function_pointers']} & {row['pointer_fields']} & {row['casts']} & "
            f"{row['implicit_casts']} & {row['to_void_pointer_casts']} & "
            f"{row['from_void_pointer_casts']} & {row['byte_pointer_casts']} \\\\"
        )
    total = {field: sum(int(row[field]) for row in summary_rows) for field in COUNT_FIELDS}
    lines.extend([
        r"\midrule",
        f"Total & {total['pointers']} & {total['void_pointers']} & {total['function_pointers']} & "
        f"{total['pointer_fields']} & {total['casts']} & {total['implicit_casts']} & "
        f"{total['to_void_pointer_casts']} & {total['from_void_pointer_casts']} & "
        f"{total['byte_pointer_casts']} \\\\"
        , r"\bottomrule", r"\end{tabular}", r"\end{table}", "",
    ])
    return "\n".join(lines)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Aggregate cast-history pointer/cast prevalence metrics.")
    parser.add_argument("--root", type=Path, required=True, help="Root recursively containing *.cast-prevalence.json sidecars")
    parser.add_argument("--out-dir", type=Path, required=True, help="Directory for CSV, JSONL, JSON, manifest, and LaTeX outputs")
    parser.add_argument("--caption", default="Pointer and cast prevalence.")
    parser.add_argument("--label", default="tab:pointer-prevalence")
    parser.add_argument("--strict", action="store_true", help="Treat malformed sidecars and duplicate conflicts as fatal")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    out_dir = args.out_dir.resolve()
    paths = discover(root)
    if not paths:
        print(f"error: no *.cast-prevalence.json files under {root}", file=sys.stderr)
        return 2
    out_dir.mkdir(parents=True, exist_ok=True)

    errors: List[str] = []
    documents: List[Tuple[Path, Mapping[str, Any]]] = []
    for path in paths:
        try:
            with path.open(encoding="utf-8",errors='replace') as handle:
                data = json.load(handle)
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: cannot read JSON: {exc}")
            continue
        if not isinstance(data, Mapping):
            errors.append(f"{path}: top-level JSON must be an object")
            continue
        errors.extend(valid_document(path, data))
        documents.append((path, data))

    per_project_declarations: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    per_project_casts: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    per_project_function_positions: Dict[str, Dict[str, Tuple[Mapping[str, Any], Path]]] = defaultdict(dict)
    declaration_rows: List[Dict[str, Any]] = []
    function_position_rows: List[Dict[str, Any]] = []
    cast_rows: List[Dict[str, Any]] = []
    metric_file_count: Dict[str, int] = defaultdict(int)

    for path, data in documents:
        project = project_name(data, path, root)
        metric_file_count[project] += 1
        declarations = data.get("pointerDeclarations", [])
        casts = data.get("casts", [])
        function_positions = data.get("functionTypeVoidPointerPositions", [])
        if isinstance(declarations, list):
            for record in declarations:
                if not isinstance(record, Mapping):
                    errors.append(f"{path}: non-object pointer declaration row")
                    continue
                merge_unique(per_project_declarations[project], record, path, "pointer declaration", errors)
        if isinstance(casts, list):
            for record in casts:
                if not isinstance(record, Mapping):
                    errors.append(f"{path}: non-object cast row")
                    continue
                merge_unique(per_project_casts[project], record, path, "cast", errors)
        if isinstance(function_positions, list):
            for record in function_positions:
                if not isinstance(record, Mapping):
                    errors.append(f"{path}: non-object function-type void-pointer position row")
                    continue
                merge_unique(per_project_function_positions[project], record, path,
                             "function-type void-pointer position", errors)

    projects = sorted(
        per_project_declarations.keys() |
        per_project_casts.keys() |
        per_project_function_positions.keys()
    )
    for project in projects:
        for record, metrics_path in sorted(per_project_declarations[project].values(), key=lambda item: str(item[0].get("id", ""))):
            declaration_rows.append(to_declaration_row(project, metrics_path, record))
        for record, metrics_path in sorted(per_project_function_positions[project].values(), key=lambda item: str(item[0].get("id", ""))):
            function_position_rows.append(to_function_type_position_row(project, metrics_path, record))
        for record, metrics_path in sorted(per_project_casts[project].values(), key=lambda item: str(item[0].get("id", ""))):
            cast_rows.append(to_cast_row(project, metrics_path, record))

    summary_rows: List[Dict[str, Any]] = []
    for project in projects:
        declarations = [record for record, _ in per_project_declarations[project].values()]
        casts = [record for record, _ in per_project_casts[project].values()]
        function_positions = [record for record, _ in per_project_function_positions[project].values()]
        row: Dict[str, Any] = {"project": project, **numeric_counts(declarations, casts, function_positions)}
        row["metric_files"] = metric_file_count[project]
        row["unique_pointer_declaration_rows"] = len(declarations)
        row["unique_function_type_void_pointer_position_rows"] = len(function_positions)
        row["unique_cast_rows"] = len(casts)
        summary_rows.append(row)

    write_csv(out_dir / "pointer_declaration_rows.csv", DECLARATION_FIELDS, declaration_rows)
    write_jsonl(out_dir / "pointer_declaration_rows.jsonl", declaration_rows)
    write_csv(out_dir / "function_type_void_pointer_position_rows.csv", FUNCTION_TYPE_POSITION_FIELDS, function_position_rows)
    write_jsonl(out_dir / "function_type_void_pointer_position_rows.jsonl", function_position_rows)
    write_csv(out_dir / "cast_rows.csv", CAST_FIELDS, cast_rows)
    write_jsonl(out_dir / "cast_rows.jsonl", cast_rows)
    write_csv(out_dir / "pointer_prevalence_by_project.csv", SUMMARY_FIELDS, summary_rows)
    (out_dir / "pointer_prevalence_by_project.json").write_text(
        json.dumps(summary_rows, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (out_dir / "pointer_prevalence.tex").write_text(
        render_latex(summary_rows, args.caption, args.label), encoding="utf-8"
    )

    total = {field: sum(int(row[field]) for row in summary_rows) for field in COUNT_FIELDS}
    manifest = {
        "schemaName": "cast-history-prevalence-aggregation",
        "schemaVersion": 2,
        "toolVersion": VERSION,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "inputRoot": root.as_posix(),
        "metricFiles": [path.as_posix() for path in paths],
        "projects": [row["project"] for row in summary_rows],
        "totals": total,
        "warningsOrErrors": errors,
    }
    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    for error in errors:
        print(f"warning: {error}", file=sys.stderr)
    if args.strict and errors:
        return 1
    print(f"aggregated {len(paths)} metrics sidecars across {len(summary_rows)} project(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

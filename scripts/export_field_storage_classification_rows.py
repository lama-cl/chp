#!/usr/bin/env python3
"""Export Phase-I resolved field-storage classifications.

C6g5f4f keeps the analyzer's existing field-sensitive EntityKey semantics but
makes each resolved storage instance visible in paper facts. For example,
``f::action.ActionNode`` is distinct from the aggregate declaration
``struct sAction::ActionNode``. This tool filters the flattened paper-subject
facts and emits one row per observed instance, plus a deliberately
non-prescriptive aggregate summary grouped by declaring field.

The grouped file is an audit summary only: a mixture of instance categories is
*not* a decision to rewrite the C field to one Rust type. Field-definition
specialization belongs to Phase II.

Example:
  python3 scripts/export_field_storage_classification_rows.py \
      --paper-subject-facts results/paper-facts/paper_subject_facts.csv \
      --out-dir results/field-storage
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Mapping, Sequence

VERSION = "v1-c6g5f4f-field-storage-instance-export"

ROW_FIELDS = (
    "project", "paper_facts_file", "subject_id", "root_subject_id", "entity",
    "source_file", "source_line", "source_column",
    "function_qualified_name", "function_signature_key",
    "field_storage_scope", "field_storage_base_entity", "field_storage_base_subject_id",
    "field_storage_location_file", "field_storage_location_line", "field_storage_location_column",
    "field_storage_path", "field_storage_declaring_field", "field_storage_declaring_record",
    "field_storage_declaring_type", "field_storage_declaring_canonical_type",
    "field_storage_declaring_file", "field_storage_declaring_line", "field_storage_declaring_column",
    "declared_type", "declared_canonical_type", "outer_pointer_kind", "outer_void_pointer",
    "has_final_history", "primary_category", "behavior", "history_shape", "history_risk",
    "history_depth", "history_parallel", "history_guarded", "history_pointer_arithmetic",
    "transport_only", "deref_read", "deref_write_or_readwrite", "pointer_arithmetic",
    "array_index", "external_escape", "callback_mediated_active",
    "active_concrete_types", "active_byte_types", "active_write_concrete_types",
    "active_guards", "guarded_active_multi_target", "unguarded_active_multi_target",
    "incompatible_active_writes", "integer_pointer_flow", "byte_oriented_active_access",
    "operation_kinds",
)

SUMMARY_FIELDS = (
    "project", "field_storage_declaring_record", "field_storage_declaring_field",
    "field_storage_declaring_type", "outer_pointer_kind",
    "instance_rows", "instances_with_final_history", "void_pointer_instances",
    "distinct_primary_categories", "primary_categories", "distinct_active_concrete_types",
    "active_concrete_types", "aggregation_scope",
)

REQUIRED = {
    "project", "subject_id", "entity", "subject_kind", "field_storage_scope",
    "field_storage_base_entity", "field_storage_path", "field_storage_declaring_field",
    "declared_type", "outer_pointer_kind", "has_final_history", "primary_category",
}


def bool_text(value: object) -> str:
    if isinstance(value, bool):
        return "yes" if value else "no"
    return str(value or "")


def split_pipe(value: object) -> List[str]:
    return sorted({piece.strip() for piece in str(value or "").split("|") if piece.strip()})


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: bool_text(row.get(field, "")) for field in fields})


def load_rows(path: Path, strict: bool) -> List[Dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        fields = set(reader.fieldnames or [])
        missing = sorted(REQUIRED - fields)
        if missing:
            raise ValueError(f"{path}: missing columns: {', '.join(missing)}")
        rows: List[Dict[str, str]] = []
        seen: set[tuple[str, str]] = set()
        for index, row in enumerate(reader, start=2):
            if row.get("subject_kind") != "field-storage":
                continue
            if row.get("field_storage_scope") != "instance":
                message = f"{path}:{index}: field-storage row has non-instance scope"
                if strict:
                    raise ValueError(message)
                print(f"warning: {message}", file=sys.stderr)
                continue
            key = (str(row.get("project") or ""), str(row.get("subject_id") or ""))
            if not all(key):
                raise ValueError(f"{path}:{index}: blank project/subject_id")
            if key in seen:
                raise ValueError(f"{path}:{index}: duplicate field-storage subject {key}")
            seen.add(key)
            rows.append({name: str(value or "") for name, value in row.items()})
        return rows


def summary_rows(rows: Sequence[Mapping[str, str]]) -> List[Dict[str, object]]:
    grouped: Dict[tuple[str, str, str, str, str], List[Mapping[str, str]]] = defaultdict(list)
    for row in rows:
        key = (
            row.get("project", ""),
            row.get("field_storage_declaring_record", ""),
            row.get("field_storage_declaring_field", ""),
            row.get("field_storage_declaring_type", ""),
            row.get("outer_pointer_kind", ""),
        )
        grouped[key].append(row)

    result: List[Dict[str, object]] = []
    for key in sorted(grouped):
        current = grouped[key]
        categories = sorted({row.get("primary_category", "") for row in current if row.get("primary_category")})
        types = sorted({item for row in current for item in split_pipe(row.get("active_concrete_types", ""))})
        result.append({
            "project": key[0],
            "field_storage_declaring_record": key[1],
            "field_storage_declaring_field": key[2],
            "field_storage_declaring_type": key[3],
            "outer_pointer_kind": key[4],
            "instance_rows": len(current),
            "instances_with_final_history": sum(row.get("has_final_history") == "yes" for row in current),
            "void_pointer_instances": sum(row.get("outer_pointer_kind") == "void-data-pointer" for row in current),
            "distinct_primary_categories": len(categories),
            "primary_categories": " | ".join(categories),
            "distinct_active_concrete_types": len(types),
            "active_concrete_types": " | ".join(types),
            "aggregation_scope": "observed-instance-summary-not-field-type-decision",
        })
    return result


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--paper-subject-facts", type=Path, required=True,
                        help="CSV emitted by aggregate_paper_facts.py")
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--only-void-pointer", action="store_true",
                        help="Keep only direct void-data-pointer field-storage instances")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args(argv)

    try:
        rows = load_rows(args.paper_subject_facts.resolve(), args.strict)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.only_void_pointer:
        rows = [row for row in rows if row.get("outer_pointer_kind") == "void-data-pointer"]
    rows.sort(key=lambda row: (row.get("project", ""), row.get("field_storage_declaring_field", ""),
                               row.get("entity", ""), row.get("subject_id", "")))

    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    summary = summary_rows(rows)
    write_csv(out / "field_storage_classification_rows.csv", ROW_FIELDS, rows)
    write_csv(out / "field_storage_classification_by_declaring_field.csv", SUMMARY_FIELDS, summary)
    with (out / "field_storage_classification_rows.jsonl").open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")
    manifest = {
        "schemaName": "cast-history-field-storage-classification-manifest",
        "schemaVersion": 1,
        "toolVersion": VERSION,
        "createdAt": datetime.now(timezone.utc).isoformat(),
        "paperSubjectFacts": args.paper_subject_facts.resolve().as_posix(),
        "onlyVoidPointer": args.only_void_pointer,
        "fieldStorageRows": len(rows),
        "declaringFieldGroups": len(summary),
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {len(rows)} field-storage instance rows and {len(summary)} declaring-field summaries to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

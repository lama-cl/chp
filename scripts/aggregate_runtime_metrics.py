#!/usr/bin/env python3
"""Aggregate C6g5g runtime-timing sidecars into TU and project CSV tables.

The primary performance table should normally use four columns derived from
this script: Parse/AST, Cast-history construction, Artifact write-out, and
Total.  The emitted CSV retains SCC/import and artifact subphases for diagnosis
without requiring them in the paper's main table.

Examples:
  scripts/aggregate_runtime_metrics.py out/project/runtime-metrics \
      --per-tu-csv out/project/runtime-duration-by-tu.csv \
      --per-project-csv out/project/runtime-duration-by-project.csv

  scripts/aggregate_runtime_metrics.py out/project-a/runtime-metrics \
      out/project-b/runtime-metrics \
      --per-tu-csv out/runtime-duration-by-tu.csv \
      --per-project-csv out/runtime-duration-by-project.csv
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Sequence, Tuple

SCHEMA_NAME = "cast-history-tu-runtime-timing"
SCHEMA_VERSION = 1

TIMING_FIELDS = (
    "parseAndAstConstruction",
    "astTraversal",
    "sameTUNamedCallSCCFixpoint",
    "crossTUSCCFixpoint",
    "importedSummaryOverlay",
    "castHistoryConstruction",
    "stdoutReportWrite",
    "prevalenceMetricsWrite",
    "linkSummaryWrite",
    "verboseReportWrite",
    "paperFactsWrite",
    "artifactWrite",
    "totalElapsed",
    "unattributed",
)

PER_TU_FIELDS = (
    "project", "stage", "source_file", "configuration_id", "timing_file",
    "parse_ast_ms", "cast_history_construction_ms", "artifact_write_ms",
    "total_elapsed_ms", "unattributed_ms", "ast_traversal_ms",
    "same_tu_named_scc_fixpoint_ms", "cross_tu_scc_fixpoint_ms",
    "imported_summary_overlay_ms", "stdout_report_write_ms",
    "prevalence_metrics_write_ms", "link_summary_write_ms",
    "verbose_report_write_ms", "paper_facts_write_ms",
)

PER_PROJECT_FIELDS = (
    "project", "stage", "translation_units", "parse_ast_ms",
    "cast_history_construction_ms", "artifact_write_ms", "total_elapsed_sum_ms",
    "total_elapsed_max_ms", "unattributed_ms", "ast_traversal_ms",
    "same_tu_named_scc_fixpoint_ms", "cross_tu_scc_fixpoint_ms",
    "imported_summary_overlay_ms", "stdout_report_write_ms",
    "prevalence_metrics_write_ms", "link_summary_write_ms",
    "verbose_report_write_ms", "paper_facts_write_ms",
)


def as_object(value: Any) -> Mapping[str, Any]:
    return value if isinstance(value, Mapping) else {}


def nonnegative_int(value: Any, field: str, path: Path, errors: List[str]) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        errors.append(f"{path}: timingsMilliseconds.{field} must be a non-negative integer")
        return 0
    return value


def discover(inputs: Sequence[Path]) -> List[Path]:
    found: List[Path] = []
    for item in inputs:
        if item.is_file():
            if item.name.endswith(".cast-runtime.json"):
                found.append(item)
            continue
        if item.is_dir():
            found.extend(path for path in item.rglob("*.cast-runtime.json") if path.is_file())
    return sorted(set(found))


def load_record(path: Path, project_override: str) -> Tuple[Dict[str, Any] | None, List[str]]:
    errors: List[str] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return None, [f"{path}: invalid JSON: {exc}"]
    if not isinstance(data, Mapping):
        return None, [f"{path}: top-level JSON must be an object"]
    if data.get("schemaName") != SCHEMA_NAME:
        errors.append(f"{path}: schemaName must be {SCHEMA_NAME!r}")
    if data.get("schemaVersion") != SCHEMA_VERSION:
        errors.append(f"{path}: schemaVersion must be {SCHEMA_VERSION}")

    tu = as_object(data.get("tu"))
    timings = as_object(data.get("timingsMilliseconds"))
    source_file = str(tu.get("sourceFile") or "")
    if not source_file:
        errors.append(f"{path}: tu.sourceFile must be nonempty")
    project = project_override or str(tu.get("projectId") or "")
    if not project:
        # Runner-created sidecars always include projectId. This fallback keeps
        # direct analyzer use useful without pretending it is project-aligned.
        project = path.parent.parent.name if path.parent.name == "runtime-metrics" else path.parent.name
    stage = str(tu.get("stage") or "") or "unspecified"

    values = {field: nonnegative_int(timings.get(field), field, path, errors)
              for field in TIMING_FIELDS}
    expected_history = (values["astTraversal"] +
                        values["sameTUNamedCallSCCFixpoint"] +
                        values["crossTUSCCFixpoint"] +
                        values["importedSummaryOverlay"])
    if values["castHistoryConstruction"] != expected_history:
        errors.append(
            f"{path}: castHistoryConstruction={values['castHistoryConstruction']} "
            f"does not equal its documented subphase sum={expected_history}")
    expected_artifacts = (values["stdoutReportWrite"] + values["prevalenceMetricsWrite"] +
                          values["linkSummaryWrite"] + values["verboseReportWrite"] +
                          values["paperFactsWrite"])
    if values["artifactWrite"] != expected_artifacts:
        errors.append(
            f"{path}: artifactWrite={values['artifactWrite']} does not equal "
            f"its documented subphase sum={expected_artifacts}")

    record: Dict[str, Any] = {
        "project": project,
        "stage": stage,
        "source_file": source_file,
        "configuration_id": str(tu.get("configurationId") or ""),
        "timing_file": str(path),
        "parse_ast_ms": values["parseAndAstConstruction"],
        "cast_history_construction_ms": values["castHistoryConstruction"],
        "artifact_write_ms": values["artifactWrite"],
        "total_elapsed_ms": values["totalElapsed"],
        "unattributed_ms": values["unattributed"],
        "ast_traversal_ms": values["astTraversal"],
        "same_tu_named_scc_fixpoint_ms": values["sameTUNamedCallSCCFixpoint"],
        "cross_tu_scc_fixpoint_ms": values["crossTUSCCFixpoint"],
        "imported_summary_overlay_ms": values["importedSummaryOverlay"],
        "stdout_report_write_ms": values["stdoutReportWrite"],
        "prevalence_metrics_write_ms": values["prevalenceMetricsWrite"],
        "link_summary_write_ms": values["linkSummaryWrite"],
        "verbose_report_write_ms": values["verboseReportWrite"],
        "paper_facts_write_ms": values["paperFactsWrite"],
    }
    return record, errors


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def latex_escape(value: object) -> str:
    text = str(value)
    replacements = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(ch, ch) for ch in text)


def seconds(value: object) -> str:
    return f"{int(value) / 1000.0:.2f}"


def write_latex(path: Path, rows: Sequence[Mapping[str, Any]]) -> None:
    """Write a compact paper-facing final-stage runtime table.

    The CSV remains the authoritative machine-readable output. This table uses
    summed per-TU elapsed values; it is therefore suitable for serial runner
    measurements, not a parallel wall-clock claim.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "% Generated by scripts/aggregate_runtime_metrics.py.",
        "% Durations are summed analyzer-process milliseconds, rendered in seconds.",
        r"\begin{tabular}{llrrrr}",
        r"\toprule",
        r"Project & Stage & TUs & Parse/AST (s) & Cast histories (s) & Write-out (s) & Total (s) \\",
        r"\midrule",
    ]
    for row in rows:
        lines.append(
            "{} & {} & {} & {} & {} & {} & {}".format(
                latex_escape(row["project"]),
                latex_escape(row["stage"]),
                row["translation_units"],
                seconds(row["parse_ast_ms"]),
                seconds(row["cast_history_construction_ms"]),
                seconds(row["artifact_write_ms"]),
                seconds(row["total_elapsed_sum_ms"]),
            ) + r" \\"
        )
    if len(rows) > 1:
        lines.extend([r"\midrule"])
        total_tus = sum(int(row["translation_units"]) for row in rows)
        lines.append(
            "Total & -- & {} & {} & {} & {} & {}".format(
                total_tus,
                seconds(sum(int(row["parse_ast_ms"]) for row in rows)),
                seconds(sum(int(row["cast_history_construction_ms"]) for row in rows)),
                seconds(sum(int(row["artifact_write_ms"]) for row in rows)),
                seconds(sum(int(row["total_elapsed_sum_ms"]) for row in rows)),
            ) + r" \\"
        )
    lines.extend([r"\bottomrule", r"\end{tabular}", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def project_rows(records: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    grouped: Dict[Tuple[str, str], List[Mapping[str, Any]]] = defaultdict(list)
    for record in records:
        grouped[(str(record["project"]), str(record["stage"]))].append(record)

    field_map = {
        "parse_ast_ms": "parse_ast_ms",
        "cast_history_construction_ms": "cast_history_construction_ms",
        "artifact_write_ms": "artifact_write_ms",
        "unattributed_ms": "unattributed_ms",
        "ast_traversal_ms": "ast_traversal_ms",
        "same_tu_named_scc_fixpoint_ms": "same_tu_named_scc_fixpoint_ms",
        "cross_tu_scc_fixpoint_ms": "cross_tu_scc_fixpoint_ms",
        "imported_summary_overlay_ms": "imported_summary_overlay_ms",
        "stdout_report_write_ms": "stdout_report_write_ms",
        "prevalence_metrics_write_ms": "prevalence_metrics_write_ms",
        "link_summary_write_ms": "link_summary_write_ms",
        "verbose_report_write_ms": "verbose_report_write_ms",
        "paper_facts_write_ms": "paper_facts_write_ms",
    }

    rows: List[Dict[str, Any]] = []
    for (project, stage), values in sorted(grouped.items()):
        row: Dict[str, Any] = {
            "project": project,
            "stage": stage,
            "translation_units": len(values),
            "total_elapsed_sum_ms": sum(int(item["total_elapsed_ms"]) for item in values),
            "total_elapsed_max_ms": max((int(item["total_elapsed_ms"]) for item in values), default=0),
        }
        for output_field, input_field in field_map.items():
            row[output_field] = sum(int(item[input_field]) for item in values)
        rows.append(row)
    return rows


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", type=Path,
                        help="runtime sidecar file(s) or directory/directories to scan recursively")
    parser.add_argument("--per-tu-csv", required=True, type=Path)
    parser.add_argument("--per-project-csv", required=True, type=Path)
    parser.add_argument("--latex-out", type=Path,
                        help="optional compact LaTeX table using project/stage aggregate rows")
    parser.add_argument("--project-name", default="",
                        help="override project ID for all input records (for direct single-project analyzer runs)")
    args = parser.parse_args(argv)

    files = discover(args.inputs)
    if not files:
        print("error: no *.cast-runtime.json files found", file=sys.stderr)
        return 2

    records: List[Dict[str, Any]] = []
    errors: List[str] = []
    seen: Dict[Tuple[str, str, str, str], Path] = {}
    for path in files:
        record, document_errors = load_record(path, args.project_name)
        errors.extend(document_errors)
        if record is None:
            continue
        identity = (str(record["project"]), str(record["stage"]),
                    str(record["source_file"]), str(record["configuration_id"]))
        earlier = seen.get(identity)
        if earlier is not None:
            errors.append(f"duplicate runtime timing identity {identity}: {earlier} and {path}")
            continue
        seen[identity] = path
        records.append(record)

    if errors:
        print("runtime timing validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    records.sort(key=lambda item: (str(item["project"]), str(item["stage"]),
                                   str(item["source_file"]), str(item["configuration_id"])))
    aggregate_rows = project_rows(records)
    write_csv(args.per_tu_csv, PER_TU_FIELDS, records)
    write_csv(args.per_project_csv, PER_PROJECT_FIELDS, aggregate_rows)
    if args.latex_out is not None:
        write_latex(args.latex_out, aggregate_rows)
    print(f"runtime timing aggregation: {len(records)} translation unit(s)")
    print(f"  per-TU:      {args.per_tu_csv}")
    print(f"  per-project: {args.per_project_csv}")
    if args.latex_out is not None:
        print(f"  LaTeX:       {args.latex_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

#!/usr/bin/env python3
"""Extract stable C6g5f4 classifications from verbose cast-history report JSON to CSV.

C6g5f4 compact link summaries do not carry classification tables; this script
prefers *.cast-report.json and falls back to legacy monolithic summaries.

Examples:
  python3 scripts/extract_stable_classifications.py --root test-output/multitu > classifications.csv
  python3 scripts/extract_stable_classifications.py --root test-output/real-projects --out real.csv
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, Iterable


FIELDS = [
    "summaryFile",
    "sourceFile",
    "subjectKind",
    "subject",
    "function",
    "paramIndex",
    "paramName",
    "declaredType",
    "behavior",
    "mediation",
    "family",
    "risk",
    "evidenceSourceKind",
    "defFile",
    "defStartLine",
    "defStartColumn",
    "defEndLine",
    "defEndColumn",
    "historyShape",
    "historyRisk",
    "view",
    "wildLike",
    "fields",
    "callbacks",
    "targets",
    "edges",
    "flag_dataField",
    "flag_nestedDataField",
    "flag_callback",
    "flag_externalBoundary",
    "flag_returnAlias",
    "flag_fieldCallbackCarrier",
    "flag_resolvedCallbackTarget",
    "flag_guardedCallbackSelection",
    "callbackMediationState",
]


def _report_files(directory: Path) -> list[Path]:
    reports = sorted(directory.rglob("*.cast-report.json"))
    if reports:
        return reports
    return sorted(directory.rglob("*.cast-summary.json"))


def iter_json_files(paths: Iterable[Path], *, root: Path | None) -> list[Path]:
    files: list[Path] = []
    if root is not None:
        files.extend(_report_files(root))
    for p in paths:
        if p.is_dir():
            files.extend(_report_files(p))
        elif p.name.endswith(".json"):
            files.append(p)
    seen: set[Path] = set()
    out: list[Path] = []
    for f in files:
        rp = f.resolve()
        if rp not in seen:
            seen.add(rp)
            out.append(f)
    return out


def list_str(v: Any) -> str:
    if isinstance(v, list):
        return "|".join(str(x) for x in v)
    if v is None:
        return ""
    return str(v)


def bool_cell(v: Any) -> str:
    if isinstance(v, bool):
        return "true" if v else "false"
    return ""


def int_cell(v: Any) -> str:
    if isinstance(v, bool):
        return ""
    if isinstance(v, int):
        return str(v)
    return ""


def base_row(path: Path, data: dict[str, Any]) -> dict[str, str]:
    tu = data.get("tu") if isinstance(data.get("tu"), dict) else {}
    return {
        "summaryFile": str(path),
        "sourceFile": str(tu.get("sourceFile", "")),
    }


def row_from_stable(
    path: Path,
    data: dict[str, Any],
    item: dict[str, Any],
    stable_key: str,
) -> dict[str, str]:
    stable = item.get(stable_key)
    if not isinstance(stable, dict):
        stable = {}
    flags = stable.get("flags") if isinstance(stable.get("flags"), dict) else {}
    location = stable.get("location") if isinstance(stable.get("location"), dict) else {}
    row = {k: "" for k in FIELDS}
    row.update(base_row(path, data))
    row.update({
        "subjectKind": str(stable.get("subjectKind", "")),
        "subject": str(stable.get("subject", "")),
        "function": str(item.get("function", "")),
        "paramIndex": str(item.get("paramIndex", "")),
        "paramName": str(item.get("paramName", "")),
        "declaredType": str(item.get("declaredType", "")),
        "behavior": str(stable.get("behavior", item.get("behavior", ""))),
        "mediation": str(stable.get("mediation", "")),
        "family": str(stable.get("family", "")),
        "risk": str(stable.get("risk", item.get("historyRisk", ""))),
        "evidenceSourceKind": str(stable.get("evidenceSourceKind", "")),
        "defFile": str(location.get("file", "")),
        "defStartLine": int_cell(location.get("startLine")),
        "defStartColumn": int_cell(location.get("startColumn")),
        "defEndLine": int_cell(location.get("endLine")),
        "defEndColumn": int_cell(location.get("endColumn")),
        "historyShape": str(item.get("historyShape", "")),
        "historyRisk": str(item.get("historyRisk", "")),
        "view": str(item.get("view", "")),
        "wildLike": bool_cell(item.get("wildLike")),
        "fields": list_str(stable.get("fields")),
        "callbacks": list_str(stable.get("callbacks")),
        "targets": list_str(stable.get("targets")),
        "edges": list_str(stable.get("edges")),
        "flag_dataField": bool_cell(flags.get("dataField")),
        "flag_nestedDataField": bool_cell(flags.get("nestedDataField")),
        "flag_callback": bool_cell(flags.get("callback")),
        "flag_externalBoundary": bool_cell(flags.get("externalBoundary")),
        "flag_returnAlias": bool_cell(flags.get("returnAlias")),
        "flag_fieldCallbackCarrier": bool_cell(flags.get("fieldCallbackCarrier")),
        "flag_resolvedCallbackTarget": bool_cell(flags.get("resolvedCallbackTarget")),
        "flag_guardedCallbackSelection": bool_cell(flags.get("guardedCallbackSelection")),
        "callbackMediationState": str(stable.get("callbackMediationState", item.get("callbackMediationState", ""))),
    })
    return row


def extract(path: Path) -> list[dict[str, str]]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        print(f"warning: skip {path}: {exc}", file=sys.stderr)
        return []
    if not isinstance(data, dict):
        return []
    if data.get("schemaName") == "cast-history-tu-link-summary":
        print(
            f"warning: skip compact link summary {path}; stable classifications live in *.cast-report.json",
            file=sys.stderr,
        )
        return []
    rows: list[dict[str, str]] = []
    for item in data.get("pointerBehaviorMetrics", []) or []:
        if isinstance(item, dict) and "stableClassification" in item:
            rows.append(row_from_stable(path, data, item, "stableClassification"))
    for item in data.get("functionParameterBehaviorMetrics", []) or []:
        if isinstance(item, dict) and "stableParamClassification" in item:
            rows.append(row_from_stable(path, data, item, "stableParamClassification"))
    return rows


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*", type=Path)
    ap.add_argument("--root", type=Path, help="root directory to search for *.cast-report.json (or legacy summaries)")
    ap.add_argument("--out", type=Path, help="write CSV here instead of stdout")
    args = ap.parse_args(argv)

    files = iter_json_files(args.paths, root=args.root)
    rows: list[dict[str, str]] = []
    for path in files:
        rows.extend(extract(path))

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        fh = args.out.open("w", newline="", encoding="utf-8")
        close = True
    else:
        fh = sys.stdout
        close = False
    try:
        writer = csv.DictWriter(fh, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if close:
            fh.close()
    print(f"extracted {len(rows)} stable classification row(s) from {len(files)} file(s)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
#!/usr/bin/env python3
"""Validate C6g5e compact link and verbose report JSON shape.

The script is dependency-light.  If the optional `jsonschema` package is
available, it validates verbose report/legacy summaries against
schemas/cast-summary.schema.json. Compact link artifacts use their deliberately
smaller structural interchange contract. Otherwise it falls back to structural
checks for both C6g5e artifact kinds.

Examples:
  python3 scripts/validate_cast_summary_schema.py --root test-output/multitu
  python3 scripts/validate_cast_summary_schema.py test-output/multitu/p77/summaries/a.c.cast-summary.json test-output/multitu/p77/reports/a.c.cast-report.json
  python3 scripts/validate_cast_summary_schema.py --strict --root test-output/multitu
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Iterable


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_SCHEMA = SCRIPT_DIR.parent / "schemas" / "cast-summary.schema.json"


class Reporter:
    def __init__(self, *, strict: bool = False) -> None:
        self.strict = strict
        self.errors = 0
        self.warnings = 0

    def error(self, path: Path, message: str) -> None:
        self.errors += 1
        print(f"ERROR: {path}: {message}", file=sys.stderr)

    def warn(self, path: Path, message: str) -> None:
        if self.strict:
            self.error(path, message)
            return
        self.warnings += 1
        print(f"WARN: {path}: {message}", file=sys.stderr)


def iter_json_files(paths: Iterable[Path], *, root: Path | None) -> list[Path]:
    files: list[Path] = []
    if root is not None:
        files.extend(sorted(root.rglob("*.cast-summary.json")))
        files.extend(sorted(root.rglob("*.cast-report.json")))
    for p in paths:
        if p.is_dir():
            files.extend(sorted(p.rglob("*.cast-summary.json")))
            files.extend(sorted(p.rglob("*.cast-report.json")))
        elif p.name.endswith(".json"):
            files.append(p)
    # Preserve order while de-duplicating.
    seen: set[Path] = set()
    out: list[Path] = []
    for f in files:
        rp = f.resolve()
        if rp not in seen:
            seen.add(rp)
            out.append(f)
    return out


def load_json(path: Path, reporter: Reporter) -> Any | None:
    try:
        with path.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception as exc:  # noqa: BLE001
        reporter.error(path, f"could not parse JSON: {exc}")
        return None


def is_bool(v: Any) -> bool:
    return isinstance(v, bool)


def is_string(v: Any) -> bool:
    return isinstance(v, str)


def is_string_list(v: Any) -> bool:
    return isinstance(v, list) and all(isinstance(x, str) for x in v)


def check_flags(path: Path, where: str, flags: Any, reporter: Reporter) -> None:
    if not isinstance(flags, dict):
        reporter.error(path, f"{where}: flags must be object")
        return
    for key in ["dataField", "nestedDataField", "callback", "externalBoundary", "returnAlias"]:
        if key not in flags:
            reporter.warn(path, f"{where}: missing flags.{key}")
        elif not is_bool(flags[key]):
            reporter.error(path, f"{where}: flags.{key} must be boolean")


def check_semantic_mediation(path: Path, where: str, sm: Any, reporter: Reporter) -> None:
    if not isinstance(sm, dict):
        reporter.error(path, f"{where}: semanticMediation must be object")
        return
    for key in ["kind", "semanticFamily"]:
        if key not in sm:
            reporter.error(path, f"{where}: semanticMediation missing {key}")
        elif not is_string(sm[key]) or not sm[key]:
            reporter.error(path, f"{where}: semanticMediation.{key} must be non-empty string")
    for key in ["fields", "callbacks", "targets", "edges", "sources"]:
        if key in sm and not is_string_list(sm[key]):
            reporter.error(path, f"{where}: semanticMediation.{key} must be list[str]")
    check_flags(path, f"{where}: semanticMediation", sm.get("flags"), reporter)


def check_stable(path: Path, where: str, sc: Any, reporter: Reporter) -> None:
    if not isinstance(sc, dict):
        reporter.error(path, f"{where}: stable classification must be object")
        return
    required_strings = [
        "subjectKind",
        "subject",
        "behavior",
        "mediation",
        "family",
        "risk",
        "evidenceSourceKind",
    ]
    for key in required_strings:
        if key not in sc:
            reporter.error(path, f"{where}: missing {key}")
        elif not is_string(sc[key]) or not sc[key]:
            reporter.error(path, f"{where}: {key} must be non-empty string")
    for key in ["fields", "callbacks", "targets", "edges"]:
        if key in sc and not is_string_list(sc[key]):
            reporter.error(path, f"{where}: {key} must be list[str]")
    check_flags(path, where, sc.get("flags"), reporter)


def fallback_validate(path: Path, data: Any, reporter: Reporter) -> None:
    if not isinstance(data, dict):
        reporter.error(path, "top-level JSON must be object")
        return

    schema_name = data.get("schemaName")
    if schema_name == "cast-history-tu-link-summary":
        for key in ["schemaVersion", "tu", "functionSummaryDetails",
                    "summaryDependencyEdges", "summaryParamTransferEdges",
                    "returnAliasPointerUseContinuations"]:
            if key not in data:
                reporter.error(path, f"link summary missing {key}")
        if not isinstance(data.get("tu"), dict) or not data["tu"].get("sourceFile"):
            reporter.warn(path, "link summary missing tu.sourceFile")
        details = data.get("functionSummaryDetails")
        if not isinstance(details, list):
            reporter.error(path, "link functionSummaryDetails must be array")
        else:
            for i, detail in enumerate(details):
                if not isinstance(detail, dict):
                    reporter.error(path, f"functionSummaryDetails[{i}] must be object")
                    continue
                if not isinstance(detail.get("identity"), dict):
                    reporter.error(path, f"functionSummaryDetails[{i}].identity must be object")
                if not isinstance(detail.get("params"), list):
                    reporter.error(path, f"functionSummaryDetails[{i}].params must be array")
        return

    if schema_name not in {"cast-history-tu-report", "cast-history-tu-summary"}:
        reporter.warn(path, "schemaName is not cast-history-tu-report, cast-history-tu-link-summary, or legacy cast-history-tu-summary")
    if "schemaVersion" not in data:
        reporter.warn(path, "missing schemaVersion")
    if not isinstance(data.get("tu"), dict) or not data["tu"].get("sourceFile"):
        reporter.warn(path, "missing tu.sourceFile")

    pointer_metrics = data.get("pointerBehaviorMetrics", [])
    if pointer_metrics is not None and not isinstance(pointer_metrics, list):
        reporter.error(path, "pointerBehaviorMetrics must be array")
    for i, item in enumerate(pointer_metrics if isinstance(pointer_metrics, list) else []):
        where = f"pointerBehaviorMetrics[{i}]"
        if not isinstance(item, dict):
            reporter.error(path, f"{where} must be object")
            continue
        if "semanticMediation" in item:
            check_semantic_mediation(path, where, item["semanticMediation"], reporter)
        else:
            reporter.warn(path, f"{where}: missing semanticMediation")
        if "stableClassification" in item:
            check_stable(path, f"{where}.stableClassification", item["stableClassification"], reporter)
        else:
            reporter.warn(path, f"{where}: missing stableClassification")

    param_metrics = data.get("functionParameterBehaviorMetrics", [])
    if param_metrics is not None and not isinstance(param_metrics, list):
        reporter.error(path, "functionParameterBehaviorMetrics must be array")
    for i, item in enumerate(param_metrics if isinstance(param_metrics, list) else []):
        where = f"functionParameterBehaviorMetrics[{i}]"
        if not isinstance(item, dict):
            reporter.error(path, f"{where} must be object")
            continue
        if "semanticMediation" in item:
            check_semantic_mediation(path, where, item["semanticMediation"], reporter)
        else:
            reporter.warn(path, f"{where}: missing semanticMediation")
        if "stableParamClassification" in item:
            check_stable(path, f"{where}.stableParamClassification", item["stableParamClassification"], reporter)
        else:
            reporter.warn(path, f"{where}: missing stableParamClassification")


def jsonschema_validate(files: list[Path], schema_path: Path, reporter: Reporter) -> bool:
    try:
        import jsonschema  # type: ignore
        from referencing import Registry, Resource  # type: ignore
    except Exception:
        return False

    try:
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
        stable_schema_path = schema_path.parent / "stable-classification.schema.json"
        stable_schema = json.loads(stable_schema_path.read_text(encoding="utf-8"))
        registry = Registry().with_resources([
            (schema.get("$id", "cast-summary.schema.json"), Resource.from_contents(schema)),
            (stable_schema.get("$id", "stable-classification.schema.json"), Resource.from_contents(stable_schema)),
            ("stable-classification.schema.json", Resource.from_contents(stable_schema)),
        ])
        validator = jsonschema.Draft202012Validator(schema, registry=registry)
    except Exception as exc:  # noqa: BLE001
        reporter.error(schema_path, f"could not load schema: {exc}")
        return True

    for path in files:
        data = load_json(path, reporter)
        if data is None:
            continue
        if isinstance(data, dict) and data.get("schemaName") == "cast-history-tu-link-summary":
            # The link artifact intentionally has a smaller contract than the
            # stable classification/report schema.  Validate it structurally.
            fallback_validate(path, data, reporter)
            continue
        for err in sorted(validator.iter_errors(data), key=lambda e: list(e.path)):
            loc = "/".join(str(p) for p in err.path)
            reporter.error(path, f"schema violation at /{loc}: {err.message}")
    return True


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*", type=Path, help="summary JSON files or directories")
    ap.add_argument("--root", type=Path, help="root directory to search for C6g5e *.cast-summary.json and *.cast-report.json")
    ap.add_argument("--schema", type=Path, default=DEFAULT_SCHEMA)
    ap.add_argument("--no-jsonschema", action="store_true", help="force stdlib structural checks")
    ap.add_argument("--strict", action="store_true", help="treat structural warnings as errors")
    args = ap.parse_args(argv)

    files = iter_json_files(args.paths, root=args.root)
    reporter = Reporter(strict=args.strict)
    if not files:
      print("warning: no C6g5e summary/report JSON files found", file=sys.stderr)
      return 0

    used_jsonschema = False
    if not args.no_jsonschema:
        used_jsonschema = jsonschema_validate(files, args.schema, reporter)

    if not used_jsonschema:
        for path in files:
            data = load_json(path, reporter)
            if data is not None:
                fallback_validate(path, data, reporter)

    mode = "jsonschema" if used_jsonschema else "structural"
    print(
        f"validated {len(files)} file(s) with {mode} checks: "
        f"errors={reporter.errors}, warnings={reporter.warnings}"
    )
    return 1 if reporter.errors else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

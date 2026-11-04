#!/usr/bin/env python3
"""
validate_classification_labels.py

C6g5f4 validation helper for the cast-history tool.

The script validates the machine-readable JSON classification records produced
by cast-history-tool and checks that the paper-facing classification labels stay
within the C6g5f4 verbose report sidecars. Compact link summaries are excluded
from directory scans because they intentionally omit classification data.
within the frozen C6f2 vocabulary. JSON validation is the default because it is
less fragile than the human-readable text output and is what downstream
experiments should consume. Text validation is still available with
--format text or --format both for debugging.

Typical usage:

  # Validate JSON summaries under the current multitu output tree.
  # Unknown vocabulary is WARN by default; hard semantic contradictions remain ERROR.
  python3 scripts/validate_classification_labels.py \
      --root test-output/multitu

  # Freeze a freshly generated C6f2 corpus with strict vocabulary checking.
  python3 scripts/validate_classification_labels.py \
      --strict-vocabulary --root test-output/multitu

  # Validate both JSON and text files when debugging one project.
  python3 scripts/validate_classification_labels.py \
      --format both test-output/multitu/p77_classify_data_field_mediated_specialization

  # Validate a single text file explicitly.
  python3 scripts/validate_classification_labels.py \
      --format text test-output/multitu/p77_classify_data_field_mediated_specialization/a.c.txt

Exit status is 0 if no errors are found; non-zero otherwise.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Iterator


ALLOWED_MEDIATION_KINDS = {
    "Direct",
    "DataField",
    "NestedDataField",
    "DataFieldCallback",
    "NestedDataFieldCallback",
    "Callback",
    "GuardedCallback",
    "FieldCallback",
    "ResolvedCallback",
    "ResolvedFieldCallback",
    "GuardedResolvedFieldCallback",
    "ReturnAlias",
    "ExternalBoundary",
    "Mixed",
    "Unknown",
}

ALLOWED_SEMANTIC_FAMILIES = {
    # PaperWildnessSummary base families that can legitimately pass through the
    # mediation layer unchanged when the mediation kind is Direct.
    "NotWildLike",
    "ConcreteSpecialization",
    "ByteView",
    "ByteRead",
    "ByteWrite",
    "ByteViewWrite",
    "CallbackDependent",
    "FieldCallbackDependent",
    "ResolvedCallbackMediatedByteView",
    "ResolvedCallbackMediatedConcreteSpecialization",
    "ResolvedCallbackMediatedGuardedSpecialization",
    "ResolvedCallbackMediatedOpaqueTransport",
    "ResolvedCallbackMediatedByteMediatedReinterpretation",
    "ResolvedCallbackMediatedMultiTargetReinterpretation",
    "ResolvedCallbackMediatedPointer",
    "ResolvedFieldCallbackMediatedByteView",
    "ResolvedFieldCallbackMediatedConcreteSpecialization",
    "ResolvedFieldCallbackMediatedGuardedSpecialization",
    "ResolvedFieldCallbackMediatedOpaqueTransport",
    "ResolvedFieldCallbackMediatedByteMediatedReinterpretation",
    "ResolvedFieldCallbackMediatedMultiTargetReinterpretation",
    "ResolvedFieldCallbackMediatedPointer",
    "GuardedResolvedFieldCallbackMediatedByteView",
    "GuardedResolvedFieldCallbackMediatedConcreteSpecialization",
    "GuardedResolvedFieldCallbackMediatedGuardedSpecialization",
    "GuardedResolvedFieldCallbackMediatedOpaqueTransport",
    "GuardedResolvedFieldCallbackMediatedByteMediatedReinterpretation",
    "GuardedResolvedFieldCallbackMediatedMultiTargetReinterpretation",
    "GuardedResolvedFieldCallbackMediatedPointer",
    "ExternalUnknown",
    "ExternalUnknownBoundary",
    "BranchTaggedUnionLike",
    "GuardedConcreteSpecialization",
    "OpaqueTransport",
    "TypeErasure",
    "ByteMediatedReinterpretation",
    "SequentialConcreteReinterpretation",
    "MultiTargetReinterpretation",
    "UnsafeReinterpretation",
    "ReturnAliasMediatedConcreteSpecialization",
    "MixedMediatedConcreteSpecialization",
    "Unknown",

    # C6e/C6f paper-facing mediated families.
    "DirectConcreteSpecialization",
    "DataFieldMediatedConcreteSpecialization",
    "NestedDataFieldMediatedConcreteSpecialization",
    "DataFieldCallbackMediatedConcreteSpecialization",
    "NestedDataFieldCallbackMediatedConcreteSpecialization",
    "CallbackMediatedConcreteSpecialization",
    "GuardedConcreteSpecialization",
    "GuardedDataFieldMediatedSpecialization",
    "GuardedNestedDataFieldMediatedSpecialization",
    "GuardedDataFieldCallbackMediatedSpecialization",
    "GuardedCallbackMediatedSpecialization",
    "ReturnAliasMediatedConcreteSpecialization",
    "ExternalUnknownBoundary",
    "MixedMediatedConcreteSpecialization",
    "UnknownMediation",
}

ALLOWED_EDGES = {
    "direct-call",
    "recursive-direct-call",
    "same-tu-call",
    "imported-direct-call",
    "summary-param-transfer",
    "summary-return-transfer",
    "summary-return-alias-transfer",
    "summary-pointer-use-transfer",
    "cross-tu-direct-call",
    "imported-function-pointer-call",
    "imported-fnptr-call",
    "same-tu-named-call",
    "function-pointer-call",
    "field-function-pointer-call",
    "higher-order-call",
    "higher-order-callback",
    "data-field-load",
    "data-field-use",
    "data-field-alias",
    "data-field-param-call",
    "data-field-function-pointer-call",
    "data-field-field-function-pointer-call",
    "external-direct-call",
    "external-indirect-call",
    "return-alias-continuation",
    "project-scc-call",
    "project-scc-summary-transfer",
    "recursive-summary-transfer",
    "call-site-specialization",
    "unknown",
}

ALLOWED_RISK_TIERS = {
    "Low",
    "Moderate",
    "High",
    "Unknown",
    "FunctionParameterDependent",
}

ALLOWED_STABLE_KEYS = {
    "entity",
    "param",
    "function",
    "subject",
    "subjectKind",
    "behavior",
    "mediation",
    "family",
    "risk",
    "evidenceSourceKind",
    "fields",
    "targets",
    "callbacks",
    "edges",
    "flags",
    "callbackMediationState",
    "callbackFields",
}

BOOLEAN_TEXT = {"yes", "no", "true", "false", "1", "0"}

# By default the validator is compatibility-oriented: unknown vocabulary is
# reported as WARN so that older regression corpora can still be scanned for
# hard semantic invariants. Use --strict-vocabulary to turn unknown labels into
# ERRORs when freezing a new output contract.
STRICT_VOCABULARY = False


def vocab_severity() -> str:
    return "ERROR" if STRICT_VOCABULARY else "WARN"


@dataclass
class Finding:
    path: Path
    line: int
    severity: str
    message: str
    text: str = ""

    def format(self) -> str:
        loc = f"{self.path}:{self.line}" if self.line else str(self.path)
        suffix = f"\n    {self.text}" if self.text else ""
        return f"{self.severity}: {loc}: {self.message}{suffix}"


def split_csv(value: str) -> list[str]:
    value = value.strip()
    if not value or value == "<none>":
        return []
    return [part.strip() for part in value.split(",") if part.strip()]


def parse_key_values(line: str) -> dict[str, str]:
    """Parse a loose 'key=value, key=value, flags: a=yes' output line.

    Values may contain spaces.  The parser treats the next ', <identifier>='
    or ', flags:' marker as the beginning of the next field.
    """

    if ":" in line:
        _, rest = line.split(":", 1)
    else:
        rest = line
    rest = rest.strip()
    rest = rest.replace(", flags:", ", flags=")
    pattern = re.compile(r"(?:^|,\s*)([A-Za-z_][A-Za-z0-9_]*)=")
    matches = list(pattern.finditer(rest))
    result: dict[str, str] = {}
    for idx, match in enumerate(matches):
        key = match.group(1)
        start = match.end()
        end = matches[idx + 1].start() if idx + 1 < len(matches) else len(rest)
        result[key] = rest[start:end].strip().rstrip(",")
    return result


def parse_flags(flag_text: str) -> dict[str, str]:
    flags: dict[str, str] = {}
    for item in split_csv(flag_text):
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        flags[key.strip()] = value.strip()
    return flags


def field_is_nested(field: str) -> bool:
    # Strip function/entity scope prefix, e.g. run::ctx.box.payload -> ctx.box.payload.
    local = field.split("::", 1)[1] if "::" in field else field
    return local.count(".") >= 2


def validate_mediation_fields(
    path: Path, line_no: int, raw: str, kv: dict[str, str]
) -> list[Finding]:
    findings: list[Finding] = []

    kind = kv.get("kind") or kv.get("mediation")
    family = kv.get("semanticFamily") or kv.get("family")
    flags = parse_flags(kv.get("flags", ""))
    fields = split_csv(kv.get("fields", ""))
    edges = split_csv(kv.get("edges", ""))

    if kind and kind not in ALLOWED_MEDIATION_KINDS:
        findings.append(
            Finding(path, line_no, vocab_severity(), f"unknown semanticMediation kind '{kind}'", raw)
        )
    if family and family not in ALLOWED_SEMANTIC_FAMILIES:
        findings.append(
            Finding(path, line_no, vocab_severity(), f"unknown semantic family '{family}'", raw)
        )

    for edge in edges:
        if edge not in ALLOWED_EDGES:
            findings.append(Finding(path, line_no, vocab_severity(), f"unknown edge label '{edge}'", raw))

    external_boundary = flags.get("externalBoundary")
    callback = flags.get("callback")
    data_field = flags.get("dataField")
    nested_data_field = flags.get("nestedDataField")
    return_alias = flags.get("returnAlias")
    callback_state = kv.get("callbackState") or kv.get("callbackMediationState")

    allowed_callback_states = {
        "None", "FormalCallbackParameter", "UnresolvedCallback", "FieldCallback",
        "ResolvedCallback", "ResolvedFieldCallback",
        "GuardedResolvedFieldCallback", "Unknown",
    }
    if callback_state and callback_state not in allowed_callback_states:
        findings.append(
            Finding(path, line_no, vocab_severity(),
                    f"unknown callback mediation state '{callback_state}'", raw)
        )

    for flag_name, flag_value in flags.items():
        if flag_value not in BOOLEAN_TEXT:
            findings.append(
                Finding(path, line_no, "ERROR", f"flag '{flag_name}' is not yes/no-like: '{flag_value}'", raw)
            )

    if "external-direct-call" in edges and external_boundary in {"no", "false", "0"}:
        findings.append(
            Finding(
                path,
                line_no,
                "ERROR",
                "external-direct-call edge present while externalBoundary=no",
                raw,
            )
        )

    if kind == "ExternalBoundary" and external_boundary in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", "ExternalBoundary kind requires externalBoundary=yes", raw)
        )

    callback_kinds = {
        "Callback", "GuardedCallback", "FieldCallback", "ResolvedCallback",
        "ResolvedFieldCallback", "GuardedResolvedFieldCallback",
        "DataFieldCallback", "NestedDataFieldCallback",
    }
    if kind in callback_kinds and callback in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", f"{kind} mediation requires callback=yes", raw)
        )

    field_callback_kinds = {"FieldCallback", "ResolvedFieldCallback", "GuardedResolvedFieldCallback"}
    if kind in field_callback_kinds and flags.get("fieldCallbackCarrier") in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", f"{kind} mediation requires fieldCallbackCarrier=yes", raw)
        )
    resolved_callback_kinds = {"ResolvedCallback", "ResolvedFieldCallback", "GuardedResolvedFieldCallback"}
    if kind in resolved_callback_kinds and flags.get("resolvedCallbackTarget") in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", f"{kind} mediation requires resolvedCallbackTarget=yes", raw)
        )
    if kind == "GuardedResolvedFieldCallback" and flags.get("guardedCallbackSelection") in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", "GuardedResolvedFieldCallback requires guardedCallbackSelection=yes", raw)
        )

    data_field_kinds = {"DataField", "NestedDataField", "DataFieldCallback", "NestedDataFieldCallback"}
    if kind in data_field_kinds:
        if data_field in {"no", "false", "0"}:
            findings.append(
                Finding(path, line_no, "ERROR", f"{kind} mediation requires dataField=yes", raw)
            )
        if not fields:
            findings.append(
                Finding(
                    path,
                    line_no,
                    vocab_severity(),
                    f"{kind} mediation has no fields=... metadata",
                    raw,
                )
            )

    nested_data_field_kinds = {"NestedDataField", "NestedDataFieldCallback"}
    if kind in nested_data_field_kinds:
        if nested_data_field in {"no", "false", "0"}:
            findings.append(
                Finding(path, line_no, "ERROR", f"{kind} requires nestedDataField=yes", raw)
            )
        if fields and not any(field_is_nested(field) for field in fields):
            findings.append(
                Finding(
                    path,
                    line_no,
                    vocab_severity(),
                    f"{kind} has no clearly nested field path",
                    raw,
                )
            )

    if kind == "ReturnAlias" and return_alias in {"no", "false", "0"}:
        findings.append(
            Finding(path, line_no, "ERROR", "ReturnAlias mediation requires returnAlias=yes", raw)
        )

    return findings


def validate_stable_line(path: Path, line_no: int, raw: str, kv: dict[str, str]) -> list[Finding]:
    findings: list[Finding] = []
    for key in kv:
        if key not in ALLOWED_STABLE_KEYS:
            # Warning rather than error: stable lines may grow, but unexpected keys should be noticed.
            findings.append(Finding(path, line_no, "WARN", f"unrecognized stable-classification key '{key}'", raw))

    mediation = kv.get("mediation")
    family = kv.get("family")
    risk = kv.get("risk")
    edges = split_csv(kv.get("edges", ""))

    if mediation and mediation not in ALLOWED_MEDIATION_KINDS:
        findings.append(Finding(path, line_no, vocab_severity(), f"unknown stable mediation '{mediation}'", raw))
    if family and family not in ALLOWED_SEMANTIC_FAMILIES:
        findings.append(Finding(path, line_no, vocab_severity(), f"unknown stable family '{family}'", raw))
    if risk and risk not in ALLOWED_RISK_TIERS:
        findings.append(Finding(path, line_no, vocab_severity(), f"unknown stable risk '{risk}'", raw))
    for edge in edges:
        if edge not in ALLOWED_EDGES:
            findings.append(Finding(path, line_no, vocab_severity(), f"unknown stable edge '{edge}'", raw))
    return findings


def validate_text(path: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return [Finding(path, 0, "ERROR", f"cannot read file: {exc}")]

    for idx, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("semanticMediation:"):
            findings.extend(validate_mediation_fields(path, idx, stripped, parse_key_values(stripped)))
        elif stripped.startswith("stableClassification:") or stripped.startswith("stableParamClassification:"):
            kv = parse_key_values(stripped)
            findings.extend(validate_stable_line(path, idx, stripped, kv))
            # Stable lines use mediation/family rather than kind/semanticFamily; reuse invariant checks.
            findings.extend(validate_mediation_fields(path, idx, stripped, kv))
    return findings


def walk_json_objects(value: Any) -> Iterator[dict[str, Any]]:
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from walk_json_objects(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk_json_objects(child)


def json_to_kv(obj: dict[str, Any]) -> dict[str, str]:
    kv: dict[str, str] = {}
    for key, value in obj.items():
        if isinstance(value, bool):
            kv[key] = "yes" if value else "no"
        elif isinstance(value, (str, int, float)) or value is None:
            kv[key] = "" if value is None else str(value)
        elif isinstance(value, list):
            kv[key] = ", ".join(str(v) for v in value)
        elif isinstance(value, dict) and key == "flags":
            kv[key] = ", ".join(
                f"{flag}={'yes' if flag_value is True else 'no' if flag_value is False else flag_value}"
                for flag, flag_value in value.items()
            )
    return kv


def validate_json(path: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
    except (OSError, json.JSONDecodeError) as exc:
        return [Finding(path, 0, "ERROR", f"cannot parse JSON: {exc}")]

    for obj in walk_json_objects(data):
        if "semanticMediation" in obj and isinstance(obj["semanticMediation"], dict):
            kv = json_to_kv(obj["semanticMediation"])
            findings.extend(validate_mediation_fields(path, 0, "<json semanticMediation>", kv))
        if "stableClassification" in obj and isinstance(obj["stableClassification"], dict):
            kv = json_to_kv(obj["stableClassification"])
            findings.extend(validate_stable_line(path, 0, "<json stableClassification>", kv))
            findings.extend(validate_mediation_fields(path, 0, "<json stableClassification>", kv))
        if "stableParamClassification" in obj and isinstance(obj["stableParamClassification"], dict):
            kv = json_to_kv(obj["stableParamClassification"])
            findings.extend(validate_stable_line(path, 0, "<json stableParamClassification>", kv))
            findings.extend(validate_mediation_fields(path, 0, "<json stableParamClassification>", kv))
    return findings


def file_matches_format(path: Path, fmt: str) -> bool:
    if fmt == "both":
        return path.suffix in {".txt", ".out", ".log", ".json"}
    if fmt == "json":
        return path.suffix == ".json"
    if fmt == "text":
        return path.suffix in {".txt", ".out", ".log"}
    raise ValueError(f"unknown format: {fmt}")


def _visible_files(root: Path, pattern: str) -> list[Path]:
    return [
        path for path in root.rglob(pattern)
        if path.is_file() and not any(part.startswith(".") for part in path.relative_to(root).parts)
    ]


def candidate_files(root: Path, fmt: str) -> Iterator[Path]:
    if root.is_file():
        if file_matches_format(root, fmt):
            yield root
        return

    if fmt in {"json", "both"}:
        # C6g5e: classifications live only in verbose report sidecars.  Fall
        # back to legacy monolithic summaries for old output trees, but never
        # walk compact link summaries when reports are present.
        reports = _visible_files(root, "*.cast-report.json")
        json_files = reports if reports else _visible_files(root, "*.cast-summary.json")
        for path in json_files:
            yield path

    if fmt in {"text", "both"}:
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if any(part.startswith(".") for part in path.relative_to(root).parts):
                continue
            if path.suffix in {".txt", ".out", ".log"}:
                yield path

def validate_path(path: Path) -> list[Finding]:
    if path.suffix == ".json":
        return validate_json(path)
    return validate_text(path)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", type=Path, help="files or directories to validate")
    parser.add_argument(
        "--root",
        type=Path,
        help="root directory to validate recursively, e.g. test-output/multitu",
    )
    parser.add_argument(
        "--format",
        choices=("json", "text", "both"),
        default="json",
        help=(
            "which outputs to validate under directories. Default: json. "
            "Use --format text for human-readable .txt/.out/.log files, "
            "or --format both while debugging."
        ),
    )
    parser.add_argument(
        "--strict-vocabulary",
        action="store_true",
        help=(
            "treat unknown mediation kinds/families/edges as errors. "
            "Without this flag, unknown vocabulary is reported as WARN so older "
            "regression trees can still be checked for hard invariants."
        ),
    )
    parser.add_argument(
        "--warnings-as-errors",
        action="store_true",
        help="treat warnings as errors",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="only print findings, not success summary",
    )
    args = parser.parse_args(argv)

    global STRICT_VOCABULARY
    STRICT_VOCABULARY = args.strict_vocabulary

    roots: list[Path] = []
    if args.root:
        roots.append(args.root)
    roots.extend(args.paths)
    if not roots:
        roots.append(Path("test-output/multitu"))

    files: list[Path] = []
    for root in roots:
        if not root.exists():
            print(f"ERROR: {root}: path does not exist", file=sys.stderr)
            return 2
        files.extend(candidate_files(root, args.format))

    files = sorted(set(files))
    if not files and not args.quiet:
        print(
            f"classification label validation skipped: no {args.format} files found",
            file=sys.stderr,
        )
        return 0

    findings: list[Finding] = []
    for path in files:
        findings.extend(validate_path(path))

    error_count = sum(1 for finding in findings if finding.severity == "ERROR")
    warning_count = sum(1 for finding in findings if finding.severity == "WARN")

    for finding in findings:
        if finding.severity == "WARN" and not args.warnings_as_errors:
            print(finding.format(), file=sys.stderr)
        else:
            print(finding.format(), file=sys.stderr)

    if args.warnings_as_errors:
        error_count += warning_count

    if error_count:
        print(
            f"classification label validation failed: files={len(files)}, errors={error_count}, warnings={warning_count}",
            file=sys.stderr,
        )
        return 1

    if not args.quiet:
        print(
            f"classification label validation passed: files={len(files)}, errors=0, warnings={warning_count}, strictVocabulary={args.strict_vocabulary}",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

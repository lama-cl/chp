#!/usr/bin/env python3
"""
Validate consistency between cast-history text output (*.txt) and verbose TU report
JSON sidecars (*.cast-report.json).

C6g5e separates compact *.cast-summary.json link artifacts (used only for C2
index construction and cross-TU import) from report JSON.  This checker always
uses report sidecars because text/report consistency is a human-facing
validation concern.

The checker is intentionally conservative: it validates the stable facts that are
useful for regression review while avoiding brittle comparisons of large nested
IR payloads.  By default it checks that every fact printed in text is present in
JSON.  With --strict-counts it also reports extra JSON facts not printed in text.

Typical use:
  python3 validate_cast_text_json_consistency.py --root test-output/multitu/p33...
  python3 validate_cast_text_json_consistency.py --text a.c.txt --json reports/a.c.cast-report.json

Exit code:
  0 = no mismatches
  1 = mismatches or missing files
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

VALIDATOR_VERSION = "v5-c6g5e-report-sidecars"


# ----------------------------- normalization -----------------------------

_WS_RE = re.compile(r"\s+")


def norm_space(s: Any) -> str:
    if s is None:
        return ""
    return _WS_RE.sub(" ", str(s).strip())


def norm_bool_text(s: str) -> Optional[bool]:
    s = norm_space(s).lower()
    if s in {"yes", "true", "1"}:
        return True
    if s in {"no", "false", "0"}:
        return False
    return None


def split_csv_types(s: str) -> List[str]:
    s = norm_space(s)
    if not s or s == "<none>":
        return []
    return sorted([norm_space(x) for x in s.split(",") if norm_space(x)])


def sorted_unique(xs: Iterable[str]) -> List[str]:
    return sorted({norm_space(x) for x in xs if norm_space(x)})


def loc_tuple_from_json(span: Any) -> Tuple[str, str, str]:
    if not isinstance(span, Mapping):
        return ("", "", "")
    return (str(span.get("file", "")), str(span.get("line", "")), str(span.get("column", "")))


def loc_tuple_from_text(loc: str) -> Tuple[str, str, str]:
    # a.c:11:3.  Keep last two colon-separated fields as line/col.
    loc = loc.strip()
    parts = loc.rsplit(":", 2)
    if len(parts) == 3:
        return (parts[0], parts[1], parts[2])
    return (loc, "", "")


def get_attach_rendered(use: Mapping[str, Any]) -> str:
    ap = use.get("attachPath")
    if isinstance(ap, Mapping):
        return norm_space(ap.get("rendered", ""))
    return ""


# ----------------------------- text parsing -----------------------------

@dataclass
class TextUse:
    entity: str
    guard: str
    attach: str
    pointer_type: str
    access_type: str
    kind: str
    detail: str
    location: Tuple[str, str, str]

    def key(self, *, include_attach: bool = False, include_location: bool = True) -> Tuple[str, ...]:
        parts: List[str] = [
            norm_space(self.entity),
            norm_space(self.kind),
            norm_space(self.pointer_type),
            norm_space(self.access_type),
            norm_space(self.detail),
        ]
        if include_attach:
            parts.append(norm_space(self.attach))
        if include_location:
            parts.extend(self.location)
        return tuple(parts)


@dataclass
class TextBehavior:
    entity: str
    view: str = ""
    history_shape: str = ""
    history_risk: str = ""
    behavior: str = ""
    wild_like: Optional[bool] = None
    paper_surface_kind: str = ""
    paper_semantic_family: str = ""
    paper_semantic_risk_tier: str = ""
    paper_interpretation: str = ""
    surface_reasons: List[str] = field(default_factory=list)
    evidence_sources: List[str] = field(default_factory=list)
    evidence_flags: Dict[str, bool] = field(default_factory=dict)
    deref_types: List[str] = field(default_factory=list)
    write_deref_types: List[str] = field(default_factory=list)


@dataclass
class TextSummary:
    path: Path
    direct_histories: Dict[str, str] = field(default_factory=dict)
    storage_histories: Dict[str, str] = field(default_factory=dict)
    pointer_uses: List[TextUse] = field(default_factory=list)
    pointer_behaviors: Dict[str, TextBehavior] = field(default_factory=dict)
    function_summary_lines: List[str] = field(default_factory=list)
    ctu_scc_fixpoint: Dict[str, Any] = field(default_factory=dict)


SECTION_HEADINGS = {
    "Function summaries:",
    "Direct cast histories (entity-oriented):",
    "Storage-location cast histories:",
    "Pointer use summaries:",
    "Pointer behavior metrics:",
    "Cross-TU SCC parameter-summary fixpoint:",
}


def is_top_heading(line: str) -> bool:
    # A report heading is unindented and ends with ':'; exclude Command/Output labels.
    return bool(line and not line.startswith(" ") and line.endswith(":"))


def section_lines(lines: Sequence[str], heading: str) -> List[str]:
    try:
        start = next(i for i, line in enumerate(lines) if line.rstrip() == heading)
    except StopIteration:
        return []
    out: List[str] = []
    for line in lines[start + 1 :]:
        if is_top_heading(line.rstrip()):
            break
        out.append(line.rstrip("\n"))
    return out


def parse_history_section(lines: Sequence[str], heading: str) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for line in section_lines(lines, heading):
        if not line.startswith("  ") or line.strip() == "<none>":
            continue
        m = re.match(r"^\s{2}(.+?)\s+:\s+(.+?)\s*$", line)
        if m:
            out[norm_space(m.group(1))] = norm_space(m.group(2))
    return out


def parse_function_summary_lines(lines: Sequence[str]) -> List[str]:
    out: List[str] = []
    for line in section_lines(lines, "Function summaries:"):
        if line.startswith("  ") and line.strip() != "<none>":
            out.append(norm_space(line))
    return out


def parse_pointer_uses(lines: Sequence[str]) -> List[TextUse]:
    out: List[TextUse] = []
    entity = ""
    use_re = re.compile(
        r"^\s*- \[(?P<guard>[^\]]+)\]\s+"
        r"attach=(?P<attach>.*?),\s+as=(?P<as>.*?)(?:,\s+access=(?P<access>.*?))?\s+:\s+"
        r"(?P<kind>\S+)\s+\{(?P<detail>.*?)\}\s+@\s+(?P<loc>.+?)\s*$"
    )
    for line in section_lines(lines, "Pointer use summaries:"):
        if line.strip() == "<none>":
            continue
        if re.match(r"^\s{2}\S", line) and not line.lstrip().startswith("-"):
            entity = norm_space(line.strip())
            continue
        m = use_re.match(line)
        if m and entity:
            out.append(
                TextUse(
                    entity=entity,
                    guard=norm_space(m.group("guard")),
                    attach=norm_space(m.group("attach")),
                    pointer_type=norm_space(m.group("as")),
                    access_type=norm_space(m.group("access") or ""),
                    kind=norm_space(m.group("kind")),
                    detail=norm_space(m.group("detail")),
                    location=loc_tuple_from_text(m.group("loc")),
                )
            )
    return out


def parse_evidence_flags(s: str) -> Dict[str, bool]:
    out: Dict[str, bool] = {}
    for part in s.split(","):
        part = part.strip()
        if not part or "=" not in part:
            continue
        k, v = part.split("=", 1)
        b = norm_bool_text(v)
        if b is not None:
            out[norm_space(k)] = b
    return out



def parse_paper_wildness_text(val: str) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for part in val.split(", "):
        if "=" not in part:
            continue
        k, v = part.split("=", 1)
        out[norm_space(k)] = norm_space(v)
    return out

def parse_pointer_behaviors(lines: Sequence[str]) -> Dict[str, TextBehavior]:
    out: Dict[str, TextBehavior] = {}
    current: Optional[TextBehavior] = None
    for line in section_lines(lines, "Pointer behavior metrics:"):
        stripped = line.strip()
        if not stripped or stripped == "<none>":
            continue
        # Entity line: exactly two spaces and no colon.
        if re.match(r"^\s{2}\S", line) and not line.startswith("    ") and " : " not in stripped:
            current = TextBehavior(entity=norm_space(stripped))
            out[current.entity] = current
            continue
        if current is None:
            continue
        if line.startswith("    ") and ":" in stripped:
            key, val = stripped.split(":", 1)
            key = norm_space(key)
            val = norm_space(val)
            if key == "view":
                current.view = val
            elif key == "historyShape":
                current.history_shape = val
            elif key == "historyRisk":
                current.history_risk = val
            elif key == "behavior":
                current.behavior = val
            elif key == "wildLike":
                current.wild_like = norm_bool_text(val)
            elif key == "paperWildness":
                pw = parse_paper_wildness_text(val)
                current.paper_surface_kind = pw.get("surfaceKind", "")
                current.paper_semantic_family = pw.get("semanticFamily", "")
                current.paper_semantic_risk_tier = pw.get("semanticRiskTier", "")
                current.paper_interpretation = pw.get("interpretation", "")
            elif key == "surfaceReasons":
                current.surface_reasons = sorted_unique(val.split(","))
            elif key == "evidenceSources":
                current.evidence_sources = sorted_unique(val.split(","))
            elif key == "evidenceFlags":
                current.evidence_flags = parse_evidence_flags(val)
            elif key == "derefTypes":
                current.deref_types = split_csv_types(val)
            elif key == "writeDerefTypes":
                current.write_deref_types = split_csv_types(val)
    return out


def parse_ctu_scc_fixpoint(lines: Sequence[str]) -> Dict[str, Any]:
    out: Dict[str, Any] = {}
    for line in section_lines(lines, "Cross-TU SCC parameter-summary fixpoint:"):
        stripped = line.strip()
        if not stripped or stripped == "<none>":
            continue
        if ":" not in stripped:
            continue
        key, val = stripped.split(":", 1)
        key = norm_space(key)
        val = norm_space(val)
        if key == "iterations":
            m = re.match(r"(\d+)\s*/\s*(\d+)", val)
            if m:
                out["iterations"] = int(m.group(1))
                out["maxIterations"] = int(m.group(2))
        elif key in {"recursive SCC edges", "changed edges"}:
            try:
                out[key.replace(" ", "_")] = int(val)
            except ValueError:
                out[key.replace(" ", "_")] = val
        elif key == "domains":
            out["domains"] = sorted_unique(val.split(","))
        else:
            out[key.replace(" ", "_")] = val
    return out


def parse_text(path: Path) -> TextSummary:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return TextSummary(
        path=path,
        direct_histories=parse_history_section(lines, "Direct cast histories (entity-oriented):"),
        storage_histories=parse_history_section(lines, "Storage-location cast histories:"),
        pointer_uses=parse_pointer_uses(lines),
        pointer_behaviors=parse_pointer_behaviors(lines),
        function_summary_lines=parse_function_summary_lines(lines),
        ctu_scc_fixpoint=parse_ctu_scc_fixpoint(lines),
    )


# ----------------------------- json extraction -----------------------------

@dataclass
class JsonUse:
    entity: str
    guard: str
    attach: str
    pointer_type: str
    access_type: str
    kind: str
    detail: str
    location: Tuple[str, str, str]
    source_labels: List[str] = field(default_factory=list)

    def key(self, *, include_attach: bool = False, include_location: bool = True) -> Tuple[str, ...]:
        parts: List[str] = [
            norm_space(self.entity),
            norm_space(self.kind),
            norm_space(self.pointer_type),
            norm_space(self.access_type),
            norm_space(self.detail),
        ]
        if include_attach:
            parts.append(norm_space(self.attach))
        if include_location:
            parts.extend(self.location)
        return tuple(parts)


@dataclass
class JsonBehavior:
    entity: str
    view: str = ""
    history_shape: str = ""
    history_risk: str = ""
    behavior: str = ""
    wild_like: Optional[bool] = None
    paper_surface_kind: str = ""
    paper_semantic_family: str = ""
    paper_semantic_risk_tier: str = ""
    paper_interpretation: str = ""
    surface_reasons: List[str] = field(default_factory=list)
    evidence_sources: List[str] = field(default_factory=list)
    evidence_flags: Dict[str, bool] = field(default_factory=dict)
    deref_types: List[str] = field(default_factory=list)
    write_deref_types: List[str] = field(default_factory=list)


@dataclass
class JsonSummary:
    path: Path
    direct_histories: Dict[str, str]
    storage_histories: Dict[str, str]
    pointer_uses: List[JsonUse]
    pointer_behaviors: Dict[str, JsonBehavior]
    ctu_scc_fixpoint: Dict[str, Any]
    raw: Dict[str, Any]


def json_use_from_record(entity: str, use: Mapping[str, Any]) -> JsonUse:
    return JsonUse(
        entity=norm_space(entity),
        guard=norm_space(use.get("guard") or "always"),
        attach=get_attach_rendered(use),
        pointer_type=norm_space(use.get("pointerType", "")),
        access_type=norm_space(use.get("accessType", "") or ""),
        kind=norm_space(use.get("kind", "")),
        detail=norm_space(use.get("detail", "")),
        location=loc_tuple_from_json(use.get("span")),
        source_labels=sorted_unique(use.get("sourceLabels", []) if isinstance(use.get("sourceLabels"), list) else []),
    )


def json_direct_histories(raw: Mapping[str, Any], key: str) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for item in raw.get(key, []) or []:
        if isinstance(item, Mapping):
            ent = norm_space(item.get("entity", ""))
            hist = norm_space(item.get("history", ""))
            if ent:
                out[ent] = hist
    return out


def json_pointer_uses(raw: Mapping[str, Any]) -> List[JsonUse]:
    out: List[JsonUse] = []
    for entry in raw.get("pointerUseSummaries", []) or []:
        if not isinstance(entry, Mapping):
            continue
        ent = norm_space(entry.get("entity", ""))
        for use in entry.get("uses", []) or []:
            if isinstance(use, Mapping):
                out.append(json_use_from_record(ent, use))
    return out


def evidence_from_behavior(item: Mapping[str, Any]) -> Tuple[List[str], Dict[str, bool]]:
    be = item.get("behaviorEvidence")
    sources: List[str] = []
    flags: Dict[str, bool] = {}
    if isinstance(be, Mapping):
        metrics = be.get("metrics")
        if isinstance(metrics, Mapping):
            sources = sorted_unique(metrics.get("sources", []) if isinstance(metrics.get("sources"), list) else [])
            jf = metrics.get("flags")
            if isinstance(jf, Mapping):
                # Map JSON field names to text field names.
                mapping = {
                    "local": "local",
                    "sameTUNamedCall": "sameTUCall",
                    "sameTURecursive": "sameTURecursive",
                    "importedDirectCall": "importedDirect",
                    "importedFunctionPointer": "importedFnPtr",
                    "projectSCC": "projectSCC",
                    "returnAliasContinuation": "returnAlias",
                }
                for jk, tk in mapping.items():
                    if jk in jf:
                        flags[tk] = bool(jf[jk])
    return sources, flags


def json_pointer_behaviors(raw: Mapping[str, Any]) -> Dict[str, JsonBehavior]:
    out: Dict[str, JsonBehavior] = {}
    for item in raw.get("pointerBehaviorMetrics", []) or []:
        if not isinstance(item, Mapping):
            continue
        ent = norm_space(item.get("entity", ""))
        if not ent:
            continue
        wm = item.get("wildPointerMetrics") if isinstance(item.get("wildPointerMetrics"), Mapping) else {}
        sources, flags = evidence_from_behavior(item)
        pw = item.get("paperWildness") if isinstance(item.get("paperWildness"), Mapping) else {}
        out[ent] = JsonBehavior(
            entity=ent,
            view=norm_space(item.get("view", "")),
            history_shape=norm_space(item.get("historyShape", "")),
            history_risk=norm_space(item.get("historyRisk", "")),
            behavior=norm_space(item.get("behavior", "")),
            wild_like=item.get("wildLike") if isinstance(item.get("wildLike"), bool) else None,
            paper_surface_kind=norm_space(pw.get("surfaceKind", "") if isinstance(pw, Mapping) else ""),
            paper_semantic_family=norm_space(pw.get("semanticFamily", "") if isinstance(pw, Mapping) else ""),
            paper_semantic_risk_tier=norm_space(pw.get("semanticRiskTier", "") if isinstance(pw, Mapping) else ""),
            paper_interpretation=norm_space(pw.get("interpretation", "") if isinstance(pw, Mapping) else ""),
            surface_reasons=sorted_unique(pw.get("surfaceReasons", []) if isinstance(pw.get("surfaceReasons"), list) else []),
            evidence_sources=sources,
            evidence_flags=flags,
            deref_types=sorted_unique(wm.get("derefTypes", []) if isinstance(wm.get("derefTypes"), list) else []),
            write_deref_types=sorted_unique(wm.get("writeDerefTypes", []) if isinstance(wm.get("writeDerefTypes"), list) else []),
        )
    return out


def json_ctu_scc_fixpoint(raw: Mapping[str, Any]) -> Dict[str, Any]:
    j = raw.get("crossTUSCCParamFixpoint")
    if not isinstance(j, Mapping):
        return {}
    return {
        "iterations": j.get("iterations"),
        "status": "converged" if j.get("converged") is True else ("not-converged" if j.get("converged") is False else ""),
        "recursive_SCC_edges": j.get("consideredEdges"),
        "changed_edges": j.get("changedEdges"),
        "domains": sorted_unique(j.get("domains", []) if isinstance(j.get("domains"), list) else []),
        "note": norm_space(j.get("note", "")),
    }


def parse_json(path: Path) -> JsonSummary:
    raw = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(raw, Mapping):
        raise ValueError("top-level JSON value is not an object")
    schema_name = norm_space(raw.get("schemaName", ""))
    if schema_name == "cast-history-tu-link-summary":
        raise ValueError(
            "received a compact C6g5e link summary; use the sibling "
            "*.cast-report.json file for text/report consistency validation"
        )
    if schema_name and schema_name not in {"cast-history-tu-summary", "cast-history-tu-report"}:
        raise ValueError(f"unsupported summary schemaName: {schema_name}")
    return JsonSummary(
        path=path,
        direct_histories=json_direct_histories(raw, "directCastHistories"),
        storage_histories=json_direct_histories(raw, "storageCastHistories"),
        pointer_uses=json_pointer_uses(raw),
        pointer_behaviors=json_pointer_behaviors(raw),
        ctu_scc_fixpoint=json_ctu_scc_fixpoint(raw),
        raw=raw,
    )


# ----------------------------- comparison -----------------------------

@dataclass
class Problem:
    severity: str
    file: str
    section: str
    message: str


@dataclass
class PairReport:
    text_path: Path
    json_path: Path
    problems: List[Problem] = field(default_factory=list)
    checked: Dict[str, int] = field(default_factory=dict)

    def add(self, section: str, message: str, severity: str = "ERROR") -> None:
        self.problems.append(Problem(severity, str(self.text_path), section, message))


def compare_map(report: PairReport, section: str, text_map: Mapping[str, str], json_map: Mapping[str, str], *, strict: bool) -> None:
    report.checked[section] = len(text_map)
    for ent, th in sorted(text_map.items()):
        jh = json_map.get(ent)
        if jh is None:
            report.add(section, f"missing JSON entity {ent!r}")
        elif norm_space(jh) != norm_space(th):
            report.add(section, f"history mismatch for {ent!r}: text={th!r}; json={jh!r}")
    if strict:
        for ent in sorted(set(json_map) - set(text_map)):
            report.add(section, f"extra JSON entity not printed in text: {ent!r}", severity="WARN")


def compare_pointer_uses(report: PairReport, text_uses: Sequence[TextUse], json_uses: Sequence[JsonUse], *, strict: bool, include_attach: bool, include_location: bool) -> None:
    section = "pointerUseSummaries"
    report.checked[section] = len(text_uses)
    tc = Counter(u.key(include_attach=include_attach, include_location=include_location) for u in text_uses)
    jc = Counter(u.key(include_attach=include_attach, include_location=include_location) for u in json_uses)
    for key, n in sorted(tc.items()):
        if jc[key] < n:
            report.add(section, f"missing JSON pointer use x{n - jc[key]}: {key}")
    if strict:
        for key, n in sorted(jc.items()):
            if tc[key] < n:
                report.add(section, f"extra JSON pointer use x{n - tc[key]}: {key}", severity="WARN")


def compare_behavior(report: PairReport, tb: Mapping[str, TextBehavior], jb: Mapping[str, JsonBehavior], *, strict: bool) -> None:
    section = "pointerBehaviorMetrics"
    report.checked[section] = len(tb)
    fields = [
        ("view", "view"),
        ("historyShape", "history_shape"),
        ("historyRisk", "history_risk"),
        ("behavior", "behavior"),
    ]
    for ent, t in sorted(tb.items()):
        j = jb.get(ent)
        if j is None:
            report.add(section, f"missing JSON behavior metrics for {ent!r}")
            continue
        for label, attr in fields:
            tv = getattr(t, attr)
            jv = getattr(j, attr)
            if tv and jv and norm_space(tv) != norm_space(jv):
                report.add(section, f"{label} mismatch for {ent!r}: text={tv!r}; json={jv!r}")
        if t.wild_like is not None and j.wild_like is not None and t.wild_like != j.wild_like:
            report.add(section, f"wildLike mismatch for {ent!r}: text={t.wild_like}; json={j.wild_like}")
        paper_fields = [
            ("paperWildness.surfaceKind", "paper_surface_kind"),
            ("paperWildness.semanticFamily", "paper_semantic_family"),
            ("paperWildness.semanticRiskTier", "paper_semantic_risk_tier"),
            ("paperWildness.interpretation", "paper_interpretation"),
        ]
        for label, attr in paper_fields:
            tv = getattr(t, attr)
            jv = getattr(j, attr)
            if tv and jv and norm_space(tv) != norm_space(jv):
                report.add(section, f"{label} mismatch for {ent!r}: text={tv!r}; json={jv!r}")
        if t.surface_reasons and j.surface_reasons and t.surface_reasons != j.surface_reasons:
            report.add(section, f"paperWildness.surfaceReasons mismatch for {ent!r}: text={t.surface_reasons}; json={j.surface_reasons}")
        if t.deref_types != j.deref_types:
            report.add(section, f"derefTypes mismatch for {ent!r}: text={t.deref_types}; json={j.deref_types}")
        if t.write_deref_types != j.write_deref_types:
            report.add(section, f"writeDerefTypes mismatch for {ent!r}: text={t.write_deref_types}; json={j.write_deref_types}")
        # C4R0: evidenceSources/evidenceFlags. Only require if printed in text.
        if t.evidence_sources and t.evidence_sources != j.evidence_sources:
            report.add(section, f"evidenceSources mismatch for {ent!r}: text={t.evidence_sources}; json={j.evidence_sources}")
        for k, tv in sorted(t.evidence_flags.items()):
            if k not in j.evidence_flags:
                report.add(section, f"evidenceFlags missing JSON flag for {ent!r}: {k}")
            elif j.evidence_flags[k] != tv:
                report.add(section, f"evidenceFlags mismatch for {ent!r}.{k}: text={tv}; json={j.evidence_flags[k]}")
    if strict:
        for ent in sorted(set(jb) - set(tb)):
            report.add(section, f"extra JSON behavior metrics not printed in text: {ent!r}", severity="WARN")


def compare_ctu_scc(report: PairReport, t: Mapping[str, Any], j: Mapping[str, Any]) -> None:
    if not t:
        return
    section = "crossTUSCCParamFixpoint"
    report.checked[section] = 1
    for key in ["iterations", "status", "domains"]:
        if key in t and key in j and t[key] != j[key]:
            report.add(section, f"{key} mismatch: text={t[key]!r}; json={j[key]!r}")


def compare_pair(text_path: Path, json_path: Path, args: argparse.Namespace) -> PairReport:
    report = PairReport(text_path=text_path, json_path=json_path)
    try:
        t = parse_text(text_path)
    except Exception as e:
        report.add("parse-text", f"failed to parse text: {e}")
        return report
    try:
        j = parse_json(json_path)
    except Exception as e:
        report.add("parse-json", f"failed to parse JSON: {e}")
        return report

    compare_map(report, "directCastHistories", t.direct_histories, j.direct_histories, strict=args.strict_counts)
    compare_map(report, "storageCastHistories", t.storage_histories, j.storage_histories, strict=args.strict_counts)
    compare_pointer_uses(
        report,
        t.pointer_uses,
        j.pointer_uses,
        strict=args.strict_counts,
        include_attach=args.strict_attach,
        include_location=not args.ignore_locations,
    )
    compare_behavior(report, t.pointer_behaviors, j.pointer_behaviors, strict=args.strict_counts)
    compare_ctu_scc(report, t.ctu_scc_fixpoint, j.ctu_scc_fixpoint)
    return report


# ----------------------------- pair discovery/reporting -----------------------------

def _is_relative_to(child: Path, parent: Path) -> bool:
    try:
        child.resolve().relative_to(parent.resolve())
        return True
    except Exception:
        return False


def _local_json_candidates_for_text(text_path: Path) -> List[Path]:
    """Return report JSON candidates that are local to exactly this text output.

    C6g5e policy: text/report validation never reads compact link summaries.
    For a text file P/foo.c.txt, the supported report locations are:

      P/reports/foo.c.cast-report.json
      P/foo.c.cast-report.json

    This deliberately does *not* use rglob().  The p01..p33 test-output tree
    contains repeated a.c/b.c basenames, so recursive basename matching can
    create cross-project false pairs.
    """
    name = text_path.name[: -len(".txt")]
    json_name = f"{name}.cast-report.json"
    candidates: List[Path] = []
    for cand in (text_path.parent / "reports" / json_name, text_path.parent / json_name):
        if cand.is_file():
            candidates.append(cand)
    return candidates

def _pair_from_text_local(text_path: Path) -> Optional[Path]:
    candidates = _local_json_candidates_for_text(text_path)
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]
    # Prefer the canonical reports/ layout if both same-dir and reports-dir
    # JSON files exist.
    canonical = text_path.parent / "reports" / (text_path.name[: -len(".txt")] + ".cast-report.json")
    if canonical in candidates:
        return canonical
    raise RuntimeError(
        f"ambiguous local JSON summaries for {text_path}: "
        + ", ".join(str(p) for p in candidates)
    )


def _project_dir_for_text(text_path: Path) -> Path:
    return text_path.parent.resolve()


def _project_dir_for_json(json_path: Path) -> Path:
    parent = json_path.parent.resolve()
    if parent.name in {"summaries", "reports"}:
        return parent.parent.resolve()
    return parent


def _same_project_pair(text_path: Path, json_path: Path) -> bool:
    return _project_dir_for_text(text_path) == _project_dir_for_json(json_path)


def find_pairs(root: Path, *, allow_global_basename_fallback: bool = False) -> List[Tuple[Path, Path]]:
    """Find text/JSON pairs recursively using strict local layout rules.

    v4 pairing policy:
      - Iterate over text files.
      - For each text file, look only in the same directory and its immediate
        summaries/ child directory.
      - Refuse any pair whose effective project directories differ.
      - Do not perform global basename fallback, even if the legacy flag is
        supplied.  Use --text/--json for intentional non-local one-off pairs.
    """
    if allow_global_basename_fallback:
        print(
            "WARNING: --allow-global-basename-fallback is ignored in v4 --root mode; "
            "use --text/--json for explicit non-local pairs.",
            file=sys.stderr,
        )

    pairs: List[Tuple[Path, Path]] = []
    skipped: List[Path] = []

    for t in sorted(p for p in root.rglob("*.txt") if p.is_file()):
        try:
            j = _pair_from_text_local(t)
        except RuntimeError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            continue
        if j is None:
            skipped.append(t)
            continue
        if not _same_project_pair(t, j):
            print(
                f"ERROR: refused cross-project pair {t} <-> {j} "
                f"(textProject={_project_dir_for_text(t)}, jsonProject={_project_dir_for_json(j)})",
                file=sys.stderr,
            )
            continue
        pairs.append((t, j))

    if not pairs and skipped:
        print(
            f"NOTE: found {len(skipped)} *.txt files but none had local JSON summaries. "
            "Expected layout: project/foo.c.txt with project/reports/foo.c.cast-report.json. "
            "Set SUMMARY_REPORTS=1 when running the analyzer workflow.",
            file=sys.stderr,
        )
    return pairs


def print_pair_plan(pairs: Sequence[Tuple[Path, Path]]) -> None:
    for text_path, json_path in pairs:
        print(f"PAIR  {text_path}  <->  {json_path}")


def print_reports(reports: Sequence[PairReport], *, show_ok: bool) -> None:
    total_errors = sum(1 for r in reports for p in r.problems if p.severity == "ERROR")
    total_warnings = sum(1 for r in reports for p in r.problems if p.severity != "ERROR")
    for r in reports:
        errs = [p for p in r.problems if p.severity == "ERROR"]
        warns = [p for p in r.problems if p.severity != "ERROR"]
        if not errs and not warns and show_ok:
            checked = ", ".join(f"{k}={v}" for k, v in sorted(r.checked.items()))
            print(f"OK  {r.text_path}  <->  {r.json_path}  ({checked})")
            continue
        if errs or warns:
            print(f"\nPAIR {r.text_path}  <->  {r.json_path}")
            for p in r.problems:
                print(f"  {p.severity}: [{p.section}] {p.message}")
    print(f"\nSummary: pairs={len(reports)}, errors={total_errors}, warnings={total_warnings}")


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group(required=False)
    src.add_argument("--root", type=Path, help="Directory containing text outputs and reports/*.cast-report.json")
    src.add_argument("--text", type=Path, help="One text output file; requires --json")
    ap.add_argument("--json", type=Path, help="JSON summary for --text")
    ap.add_argument("--strict-counts", action="store_true", help="Also warn about JSON facts that are not printed in text")
    ap.add_argument("--strict-attach", action="store_true", help="Include attachPath/attach=... in pointer-use matching. Off by default because <id>/<none> rendering can differ.")
    ap.add_argument("--ignore-locations", action="store_true", help="Ignore source locations in pointer-use matching")
    ap.add_argument("--quiet-ok", action="store_true", help="Do not print OK lines for clean pairs")
    ap.add_argument("--show-pairs", action="store_true", help="Print discovered text/JSON pairs before validation")
    ap.add_argument("--version", action="version", version=f"%(prog)s {VALIDATOR_VERSION}")
    ap.add_argument(
        "--allow-global-basename-fallback",
        action="store_true",
        help=(
            "Unsafe legacy pairing mode: if no local JSON exists, allow matching "
            "by basename anywhere under --root. Disabled by default because many "
            "multi-TU projects contain repeated a.c/b.c names."
        ),
    )
    args = ap.parse_args(argv)

    if not args.root and not args.text:
        ap.error("one of the arguments --root --text is required")

    pairs: List[Tuple[Path, Path]] = []
    if args.root:
        pairs = find_pairs(args.root, allow_global_basename_fallback=args.allow_global_basename_fallback)
        if not pairs:
            print(f"ERROR: no *.txt / *.cast-report.json pairs found under {args.root}", file=sys.stderr)
            return 1
    else:
        if not args.json:
            ap.error("--text requires --json")
        pairs = [(args.text, args.json)]

    if args.show_pairs:
        print_pair_plan(pairs)
    reports = [compare_pair(t, j, args) for t, j in pairs]
    print_reports(reports, show_ok=not args.quiet_ok)
    return 1 if any(p.severity == "ERROR" for r in reports for p in r.problems) else 0


if __name__ == "__main__":
    raise SystemExit(main())

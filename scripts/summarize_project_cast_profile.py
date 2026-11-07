#!/usr/bin/env python3
"""Project-level cast-history profile.

Reads the artifact tree produced by a cast-history runner (run_all_multitu_projects.sh,
run_all_GenC2Rust.sh, or a local cast-history-runner.sh) and produces a single
"what is this project made of" overview: pointer population, cast distribution,
stable classification, operation characteristics, cross-TU topology, analysis
health and runtime cost.

The reader is layout-agnostic. It walks the tree and classifies every JSON file by
its ``schemaName`` rather than by directory name, so it works regardless of how a
particular runner names its output directories:

    cast-history-prevalence-metrics  -> pointer / cast inventory
    cast-history-tu-report           -> stable classification (refined vocabulary)
    cast-history-tu-paper-facts      -> subject operation + declared-type facts
    cast-history-tu-link-summary     -> translation-unit inventory
    cast-history-tu-runtime-timing   -> per-TU cost
    cast-history-project-summary-index -> cross-TU topology and solver health

Project mode vs corpus mode is auto-detected: a directory that directly owns
cast-history artifacts is a project; a directory whose children do is a corpus.

Usage:
    python3 scripts/summarize_project_cast_profile.py --root test-output/real-projects/zlib
    python3 scripts/summarize_project_cast_profile.py --root test-output/real-projects \
        --out-dir results/zlib-profile
"""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable

# --------------------------------------------------------------------------
# Artifact identification
# --------------------------------------------------------------------------

SCHEMA_PREVALENCE = "cast-history-prevalence-metrics"
SCHEMA_REPORT = "cast-history-tu-report"
SCHEMA_PAPER_FACTS = "cast-history-tu-paper-facts"
SCHEMA_LINK_SUMMARY = "cast-history-tu-link-summary"
SCHEMA_RUNTIME = "cast-history-tu-runtime-timing"
SCHEMA_INDEX = "cast-history-project-summary-index"
# Older tool revisions emitted the classification layer under this name.
SCHEMA_LEGACY_SUMMARY = "cast-history-tu-summary"

CLASSIFICATION_SCHEMAS = {SCHEMA_REPORT, SCHEMA_LEGACY_SUMMARY}

# Directories that hold per-round scratch copies rather than published results.
# Counting them would multiply every figure by the number of import rounds.
# `.work` is the runner's per-phase staging tree, kept by --keep-intermediates;
# it holds a full c1-/imp01-/local-overlay- copy of every sidecar.
INTERMEDIATE_DIR_RE = re.compile(
    r"^(\.work|\.tmp-intermediates|rounds|"
    r"(c1|imp\d+|local-overlay)-[a-z-]+|c1-[a-z-]+|"
    r"import-round-\d+[a-z-]*|local-overlay[a-z0-9-]*)$"
)
INTERMEDIATE_FILE_RE = re.compile(r"^import-round-\d+-index-input\.json$")

# Sidecars of translation units that FAILED during C1. The runner publishes them
# under partial-* so the evidence is not lost, and states that they are never
# fed to the index. They describe an aborted analysis, so counting them in the
# project profile mixes incomplete data into every distribution.
PARTIAL_DIR_RE = re.compile(r"^partial-[a-z-]+$")
PARTIAL_STATUS_SUFFIX = ".cast-analysis-status.json"

# Vocabulary frozen in schemas/stable-classification.schema.json. Labels outside
# it are not errors, but a project-level report should say the vocabulary drifted.
KNOWN_RISKS = {"Low", "Moderate", "High", "FunctionParameterDependent", "Unknown"}


def _schema_known_values(schema_dir: Path, field: str) -> set[str]:
    path = schema_dir / "stable-classification.schema.json"
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return set()
    prop = (doc.get("properties") or {}).get(field) or {}
    return set(prop.get("x-c6f-known-values") or [])


# --------------------------------------------------------------------------
# Robust JSON loading
# --------------------------------------------------------------------------


class LoadIssue(Exception):
    pass


def load_json(path: Path, issues: list[str]) -> dict[str, Any] | None:
    """Load a cast-history JSON file.

    Some analyzer revisions emit a dangling ``tu.sourceFile`` string, which puts
    raw uninitialized bytes into the file. Those files are still structurally
    valid JSON, so decode with replacement rather than refusing the whole run,
    and record the defect so the health section can report it.
    """
    try:
        raw = path.read_bytes()
    except OSError as exc:
        issues.append(f"unreadable: {path.name}: {exc}")
        return None
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("utf-8", "replace")
        issues.append(f"non-UTF-8 bytes (corrupt string field): {path.name}")
    try:
        doc = json.loads(text)
    except ValueError as exc:
        issues.append(f"invalid JSON: {path.name}: {exc}")
        return None
    return doc if isinstance(doc, dict) else None


def is_corrupt_text(value: Any) -> bool:
    """True when a string carries replacement/control bytes from a dangling read."""
    if not isinstance(value, str):
        return False
    return "�" in value or any(ord(c) < 9 for c in value)


def walk_json(
    root: Path, *, include_intermediates: bool, include_partial: bool = False
) -> Iterable[Path]:
    for dirpath, dirnames, filenames in os.walk(root):
        if not include_intermediates:
            dirnames[:] = [d for d in dirnames if not INTERMEDIATE_DIR_RE.match(d)]
        if not include_partial:
            dirnames[:] = [d for d in dirnames if not PARTIAL_DIR_RE.match(d)]
        dirnames.sort()
        for name in sorted(filenames):
            if not name.endswith(".json"):
                continue
            if not include_intermediates and INTERMEDIATE_FILE_RE.match(name):
                continue
            yield Path(dirpath) / name


class Artifacts:
    """Cast-history JSON files of one project, grouped by schema."""

    def __init__(self) -> None:
        self.partial_status: list[Path] = []
        self.partial_sidecars: list[Path] = []
        self.prevalence: list[Path] = []
        self.reports: list[Path] = []
        self.paper_facts: list[Path] = []
        self.link_summaries: list[Path] = []
        self.runtime: list[Path] = []
        self.index: Path | None = None
        self.unknown: list[Path] = []

    def total(self) -> int:
        return (
            len(self.prevalence)
            + len(self.reports)
            + len(self.paper_facts)
            + len(self.link_summaries)
            + len(self.runtime)
            + (1 if self.index else 0)
        )

    def families(self) -> dict[str, int]:
        return {
            "prevalence-metrics": len(self.prevalence),
            "reports": len(self.reports),
            "paper-facts": len(self.paper_facts),
            "link-summaries": len(self.link_summaries),
            "runtime-metrics": len(self.runtime),
            "project-summary-index": 1 if self.index else 0,
        }


SUFFIX_SCHEMA = {
    ".cast-prevalence.json": SCHEMA_PREVALENCE,
    ".cast-report.json": SCHEMA_REPORT,
    ".cast-paper-facts.json": SCHEMA_PAPER_FACTS,
    ".cast-summary.json": SCHEMA_LINK_SUMMARY,
    ".cast-runtime.json": SCHEMA_RUNTIME,
}

SCHEMA_RE = re.compile(rb'"schemaName"\s*:\s*"([^"]+)"')
SCAN_LIMIT = 16 * 1024 * 1024


def peek_schema(path: Path) -> str | None:
    """Identify a cast-history artifact without parsing it.

    Report sidecars reach hundreds of megabytes, so identification must stay
    cheap. The analyzer's own file suffixes are checked first; a suffix is only
    a hint, so it is confirmed from the file's head when the head is readable.
    Files written with sorted keys put ``schemaName`` in the middle of the
    document, so fall back to the head, the tail, and finally a bounded scan.
    """
    if path.name.endswith(PARTIAL_STATUS_SUFFIX):
        return None
    for suffix, schema in SUFFIX_SCHEMA.items():
        if path.name.endswith(suffix):
            return schema
    try:
        size = path.stat().st_size
        with path.open("rb") as fh:
            head = fh.read(8192)
            match = SCHEMA_RE.search(head)
            if match:
                return match.group(1).decode("utf-8", "replace")
            if size > 8192:
                fh.seek(max(0, size - 8192))
                match = SCHEMA_RE.search(fh.read(8192))
                if match:
                    return match.group(1).decode("utf-8", "replace")
            if 8192 < size <= SCAN_LIMIT:
                fh.seek(0)
                match = SCHEMA_RE.search(fh.read(SCAN_LIMIT))
                if match:
                    return match.group(1).decode("utf-8", "replace")
    except OSError:
        return None
    return None


def collect_partial(root: Path) -> tuple[list[Path], list[Path]]:
    """Find the sidecars and status manifests of translation units that failed in C1."""
    status: list[Path] = []
    sidecars: list[Path] = []
    for child in sorted(root.iterdir()) if root.is_dir() else []:
        if not child.is_dir() or not PARTIAL_DIR_RE.match(child.name):
            continue
        for item in sorted(child.iterdir()):
            if not item.is_file() or not item.name.endswith(".json"):
                continue
            if item.name.endswith(PARTIAL_STATUS_SUFFIX):
                status.append(item)
            else:
                sidecars.append(item)
    return status, sidecars


def collect_artifacts(
    root: Path, *, include_intermediates: bool, include_partial: bool = False
) -> Artifacts:
    found = Artifacts()
    if not include_partial:
        found.partial_status, found.partial_sidecars = collect_partial(root)
    for path in walk_json(
        root,
        include_intermediates=include_intermediates,
        include_partial=include_partial,
    ):
        schema = peek_schema(path)
        if schema == SCHEMA_PREVALENCE:
            found.prevalence.append(path)
        elif schema in CLASSIFICATION_SCHEMAS:
            found.reports.append(path)
        elif schema == SCHEMA_PAPER_FACTS:
            found.paper_facts.append(path)
        elif schema == SCHEMA_LINK_SUMMARY:
            found.link_summaries.append(path)
        elif schema == SCHEMA_RUNTIME:
            found.runtime.append(path)
        elif schema == SCHEMA_INDEX:
            # Prefer the final index over the one consumed by the last round.
            if found.index is None or path.name == "project-summary-index.json":
                found.index = path
        elif schema is not None:
            found.unknown.append(path)
    return found


# Directory names runners use for the artifact families of a single project.
# They let a project directory that owns no top-level JSON still be recognised
# as a project rather than mistaken for a corpus of three "projects" called
# summaries, reports and paper-facts.
ARTIFACT_DIR_NAMES = {
    "summaries",
    "json-summaries",
    "reports",
    "paper-facts",
    "prevalence-metrics",
    "runtime-metrics",
    "metrics",
    "sidecars",
    "cast-summaries",
}


def owns_artifacts(path: Path) -> bool:
    """True when cast-history JSON files sit directly in this directory."""
    try:
        entries = sorted(path.iterdir())
    except OSError:
        return False
    for child in entries:
        if child.is_file() and child.name.endswith(".json"):
            if peek_schema(child) is not None:
                return True
    return False


def subdirectories(path: Path) -> list[Path]:
    try:
        return [
            c
            for c in sorted(path.iterdir())
            if c.is_dir() and not INTERMEDIATE_DIR_RE.match(c.name)
        ]
    except OSError:
        return []


def classify_root(path: Path) -> str:
    """Decide whether a directory is one project, a corpus of them, or empty.

    A directory that owns cast-history JSON directly is a project. Otherwise, if
    the only child directories holding artifacts are the conventional
    per-family directories, it is still a single project. Only when artifacts
    live under child directories that are not artifact families is the
    directory treated as a corpus.
    """
    if owns_artifacts(path):
        return "project"
    children = subdirectories(path)
    holders = [c for c in children if owns_artifacts(c)]
    if holders and all(c.name in ARTIFACT_DIR_NAMES for c in holders):
        return "project"
    for child in children:
        if owns_artifacts(child) or any(owns_artifacts(gc) for gc in subdirectories(child)):
            return "corpus"
    return "empty"


# --------------------------------------------------------------------------
# Profile accumulation
# --------------------------------------------------------------------------


def normalize_path(value: str) -> str:
    """Lexically normalize a source path from a sidecar.

    Done purely on the string: the analyzed tree is often not present on the
    machine that reads the artifacts, so the filesystem cannot be consulted.
    Separators are unified, duplicate slashes collapsed, and `.` / `..`
    segments resolved, so that `/p/src/../src/a.c`, `/p/./src/a.c` and
    `/p/src/a.c` become one key instead of three.
    """
    if not value:
        return ""
    value = value.replace("\\", "/")
    absolute = value.startswith("/")
    out: list[str] = []
    for part in value.split("/"):
        if not part or part == ".":
            continue
        if part == "..":
            if out and out[-1] != "..":
                out.pop()
            elif not absolute:
                out.append("..")
            continue
        out.append(part)
    joined = "/".join(out)
    return ("/" + joined) if absolute else joined


def resolve_path(value: str, base: str | None) -> str:
    """Normalize a source path, anchoring a relative one to its project root.

    Clang reports a filename exactly as the compilation database spells it, so
    a database with relative source arguments yields relative paths here. Such
    a path is meaningless on its own but is well defined relative to the
    project root the sidecar records, so anchor it there before normalizing.
    """
    if not value:
        return ""
    value = value.replace("\\", "/")
    if not value.startswith("/") and base:
        value = base.rstrip("/") + "/" + value
    return normalize_path(value)


class ProjectProfile:
    def __init__(self, name: str, root: Path) -> None:
        self.name = name
        self.root = root
        self.issues: list[str] = []
        self.notes: list[str] = []
        self.tool_versions: set[str] = set()
        self.source_root: str | None = None
        self.reports_seen = 0
        self.paper_facts_seen = 0
        self.partial_tus: list[dict[str, Any]] = []
        self.partial_sidecar_count = 0
        self.key_collisions = 0
        self.projects_seen: set[str] = set()

        # Pointer / cast inventory, deduplicated by source-located row id so a
        # declaration in a shared header counts once for the project rather
        # than once per translation unit that includes it.
        self.decls: dict[tuple, dict[str, Any]] = {}
        self.casts: dict[tuple, dict[str, Any]] = {}
        self.tu_counts_sum: Counter[str] = Counter()
        self.prevalence_tus = 0

        # Stable classification rows, deduplicated by subject identity.
        self.class_rows: dict[tuple, dict[str, Any]] = {}
        self.class_rows_raw = 0
        self.feature_diagnostics: Counter[str] = Counter()
        self.cross_tu_lookups = 0

        # Paper-facts subjects, deduplicated by subjectId.
        self.subjects: dict[tuple, dict[str, Any]] = {}
        self.subject_rows_raw = 0
        self.coverage: Counter[str] = Counter()
        self.callback_records = 0

        # Translation units and cost.
        self.tus: set[str] = set()
        self.runtime_ms: Counter[str] = Counter()
        self.runtime_tus = 0
        self.slowest_tu: tuple[str, int] | None = None

        self.index: dict[str, Any] = {}

    # -- ingestion ------------------------------------------------------

    def add_prevalence(self, doc: dict[str, Any]) -> None:
        self.prevalence_tus += 1
        self.tool_versions.add(str(doc.get("toolVersion", "")))
        project = doc.get("project") or {}
        root = normalize_path(str(project.get("root") or ""))
        project_key = str(project.get("id") or "") or root or self.name
        self.projects_seen.add(project_key)
        if root and not self.source_root:
            self.source_root = root
        for decl in doc.get("pointerDeclarations") or []:
            self._put(self.decls, self._row_key(project_key, decl, root), decl)
        for cast in doc.get("casts") or []:
            self._put(self.casts, self._row_key(project_key, cast, root), cast)
        counts = doc.get("tuCounts") or {}
        for key, value in counts.items():
            if isinstance(value, int):
                self.tu_counts_sum[key] += value
        tu = (doc.get("tu") or {}).get("sourceFile")
        if is_corrupt_text(tu):
            self.issues.append(
                "prevalence sidecar has a corrupt tu.sourceFile "
                "(analyzer writes a dangling string reference)"
            )
        elif tu:
            self.tus.add(normalize_path(str(tu)))

    @staticmethod
    def _row_key(project_key: str, record: dict[str, Any], base: str) -> tuple:
        """Identity of a declaration or cast row.

        The emitted id is `<kind>:<file>:<line>:<col>:<discriminator>`, where
        `<file>` is the path exactly as clang reported it: not normalized, and
        not unique across projects, since two projects analyzed under one root
        can hold the same relative path. Rebuild the id with the resolved,
        normalized path and pair it with the project, which keeps every
        discriminator the analyzer encoded (declarationKind for a declaration,
        castKind plus implicit/explicit for a cast) while making
        `/p/src/../src/a.c` and `/p/src/a.c` one key.
        """
        raw_id = str(record.get("id", ""))
        raw_file = str((record.get("source") or {}).get("file", ""))
        resolved = resolve_path(raw_file, base)
        if raw_file and raw_file in raw_id:
            return (project_key, raw_id.replace(raw_file, resolved, 1))
        return (project_key, raw_id)

    def _put(self, store: dict, key: tuple, record: dict[str, Any]) -> None:
        if key in store:
            if store[key] != record:
                self.key_collisions += 1
            return
        store[key] = record

    def note_partial(self, status: dict[str, Any], sidecars: int) -> None:
        self.partial_tus.append(
            {
                "sourceFile": str(status.get("sourceFile", "")),
                "stem": str(status.get("sourceStem", "")),
                "stage": str(status.get("stage", "c1")),
                "exitCode": status.get("toolExitCode"),
                "reason": str(status.get("reason", "")),
                "artifacts": len(status.get("publishedArtifacts") or []),
            }
        )
        self.partial_sidecar_count = sidecars

    def add_report(self, doc: dict[str, Any]) -> None:
        self.reports_seen += 1
        self.tool_versions.add(str(doc.get("toolVersion", "")))
        tu = (doc.get("tu") or {}).get("sourceFile")
        if tu and not is_corrupt_text(tu):
            self.tus.add(normalize_path(str(tu)))
        for metric in doc.get("pointerBehaviorMetrics") or []:
            self._add_class_row("entity", metric, metric.get("stableClassification"))
        for metric in doc.get("functionParameterBehaviorMetrics") or []:
            self._add_class_row(
                "param", metric, metric.get("stableParamClassification")
            )
        for entry in doc.get("featureDiagnostics") or []:
            if isinstance(entry, dict):
                label = entry.get("feature") or entry.get("kind") or "unknown"
                self.feature_diagnostics[str(label)] += 1
        self.cross_tu_lookups += len(doc.get("crossTUDirectCallLookups") or [])

    def _add_class_row(
        self, kind: str, metric: dict[str, Any], stable: dict[str, Any] | None
    ) -> None:
        if not isinstance(stable, dict):
            return
        self.class_rows_raw += 1
        location = stable.get("location") or {}
        file_path = resolve_path(str(location.get("file", "")), self.source_root)
        key = (
            self.name,
            stable.get("subjectKind", kind),
            stable.get("subject", ""),
            file_path,
            location.get("startLine"),
            location.get("startColumn"),
        )
        wildness = metric.get("paperWildness") or {}
        row = {
            "subjectKind": stable.get("subjectKind", kind),
            "subject": stable.get("subject", ""),
            "behavior": stable.get("behavior", "Unknown"),
            "mediation": stable.get("mediation", "Unknown"),
            "family": stable.get("family", "Unknown"),
            "risk": stable.get("risk", "Unknown"),
            "evidenceSourceKind": stable.get("evidenceSourceKind", "unknown"),
            "flags": stable.get("flags") or {},
            "file": file_path,
            "line": location.get("startLine"),
            "declaredType": metric.get("declaredType", ""),
            "function": metric.get("function", ""),
            "historyShape": metric.get("historyShape", "Unknown"),
            "historyRisk": metric.get("historyRisk", "Unknown"),
            "wildLike": bool(metric.get("wildLike")),
            "surfaceKind": wildness.get("surfaceKind", "unknown"),
            "surfaceReasons": wildness.get("surfaceReasons") or [],
            "interpretation": wildness.get("interpretation", ""),
        }
        self.class_rows.setdefault(key, row)

    def add_paper_facts(self, doc: dict[str, Any]) -> None:
        self.paper_facts_seen += 1
        self.tool_versions.add(str(doc.get("toolVersion", "")))
        tu = (doc.get("tu") or {}).get("sourceFile")
        if tu and not is_corrupt_text(tu):
            self.tus.add(normalize_path(str(tu)))
        for subject in doc.get("subjects") or []:
            self.subject_rows_raw += 1
            key = subject.get("subjectId") or subject.get("entity")
            if key:
                self.subjects.setdefault((self.name, str(key)), subject)
        for key, value in (doc.get("coverage") or {}).items():
            if isinstance(value, int) and not isinstance(value, bool):
                self.coverage[key] += value
        self.callback_records += len(doc.get("callbackUseRecords") or [])

    def add_link_summary(self, doc: dict[str, Any]) -> None:
        self.tool_versions.add(str(doc.get("toolVersion", "")))
        tu = (doc.get("tu") or {}).get("sourceFile")
        if tu and not is_corrupt_text(tu):
            self.tus.add(normalize_path(str(tu)))

    def add_runtime(self, doc: dict[str, Any]) -> None:
        self.runtime_tus += 1
        timings = doc.get("timingsMilliseconds") or {}
        for key, value in timings.items():
            if isinstance(value, (int, float)):
                self.runtime_ms[key] += int(value)
        total = timings.get("totalElapsed") or timings.get("totalElapsedMilliseconds")
        tu = (doc.get("tu") or {}).get("sourceFile")
        if isinstance(total, (int, float)) and tu and not is_corrupt_text(str(tu)):
            name = os.path.basename(normalize_path(str(tu)))
            if self.slowest_tu is None or total > self.slowest_tu[1]:
                self.slowest_tu = (name, int(total))

    def add_index(self, doc: dict[str, Any]) -> None:
        self.index = doc

    # -- derived views ---------------------------------------------------

    def owned(self, path: str) -> bool:
        """True when a source path lies inside the analyzed project root."""
        if not self.source_root or not path:
            return False
        norm = normalize_path(path)
        return norm == self.source_root or norm.startswith(self.source_root + "/")

    def pointer_population(self) -> dict[str, Any]:
        decls = list(self.decls.values())
        by_kind = Counter(d.get("declarationKind", "other") for d in decls)
        void_by_kind = Counter(
            d.get("declarationKind", "other") for d in decls if d.get("isVoidPointer")
        )
        by_file = Counter(
            os.path.basename(
                resolve_path(str((d.get("source") or {}).get("file", "")), self.source_root)
            )
            for d in decls
        )
        total = len(decls)
        voids = sum(1 for d in decls if d.get("isVoidPointer"))
        fnptrs = sum(1 for d in decls if d.get("isFunctionPointer"))
        fields = sum(1 for d in decls if d.get("isPointerField"))
        summed = self.tu_counts_sum.get("pointerDeclarations", 0)
        return {
            "declarations": total,
            "byKind": dict(by_kind.most_common()),
            "voidPointers": voids,
            "voidPointerShare": (voids / total) if total else 0.0,
            "voidByKind": dict(void_by_kind.most_common()),
            "functionPointers": fnptrs,
            "pointerFields": fields,
            "functionTypeVoidPositions": self.tu_counts_sum.get(
                "functionTypeVoidPointerPositions", 0
            ),
            "topTypes": Counter(
                str(d.get("canonicalType", "")) for d in decls
            ).most_common(12),
            "topFiles": by_file.most_common(12),
            "tuSummed": summed,
            "duplicationFactor": (summed / total) if total else 0.0,
        }

    def cast_distribution(self) -> dict[str, Any]:
        casts = list(self.casts.values())
        total = len(casts)
        explicit = [c for c in casts if not c.get("isImplicit")]
        pairs = Counter(
            f"{c.get('sourceCanonicalType','?')} -> {c.get('destinationCanonicalType','?')}"
            for c in explicit
        )
        return {
            "casts": total,
            "explicit": len(explicit),
            "implicit": total - len(explicit),
            "explicitShare": (len(explicit) / total) if total else 0.0,
            "byKind": dict(Counter(str(c.get("castKind", "?")) for c in casts).most_common()),
            "toVoidPointer": sum(1 for c in casts if c.get("toVoidPointer")),
            "fromVoidPointer": sum(1 for c in casts if c.get("fromVoidPointer")),
            "bytePointerCast": sum(1 for c in casts if c.get("bytePointerCast")),
            "pointerToPointer": sum(
                1
                for c in casts
                if c.get("sourceIsPointer") and c.get("destinationIsPointer")
            ),
            "topExplicitPairs": pairs.most_common(12),
            "topFiles": Counter(
                os.path.basename(
                    resolve_path(str((c.get("source") or {}).get("file", "")), self.source_root)
                )
                for c in explicit
            ).most_common(10),
            "tuSummed": self.tu_counts_sum.get("explicitCasts", 0)
            + self.tu_counts_sum.get("implicitCasts", 0),
        }

    def classification(self) -> dict[str, Any]:
        rows = list(self.class_rows.values())
        total = len(rows)
        owned = [r for r in rows if self.owned(r["file"])]
        external = total - len(owned)
        flags: Counter[str] = Counter()
        for row in rows:
            for key, value in (row.get("flags") or {}).items():
                if value:
                    flags[key] += 1
        wild = [r for r in rows if r["wildLike"]]
        # The paper's refinement claim: of the surface-wild subjects, how many
        # resolve to a structured, explainable family rather than true wildness?
        structured_families = {
            "DirectConcreteSpecialization",
            "ConcreteSpecialization",
            "GuardedConcreteSpecialization",
            "DataFieldMediatedConcreteSpecialization",
            "NestedDataFieldMediatedConcreteSpecialization",
            "CallbackMediatedConcreteSpecialization",
            "GuardedCallbackMediatedSpecialization",
            "DataFieldCallbackMediatedConcreteSpecialization",
            "NestedDataFieldCallbackMediatedConcreteSpecialization",
            "ReturnAliasMediatedConcreteSpecialization",
            "ByteView",
            "OpaqueTransport",
        }
        structured = [r for r in wild if r["family"] in structured_families]
        unresolved_families = {"Unknown", "UnknownMediation", "ExternalUnknownBoundary"}
        unresolved = [r for r in wild if r["family"] in unresolved_families]
        return {
            "rows": total,
            "rowsRaw": self.class_rows_raw,
            "duplicationFactor": (self.class_rows_raw / total) if total else 0.0,
            "projectOwned": len(owned),
            "external": external,
            "bySubjectKind": dict(Counter(r["subjectKind"] for r in rows).most_common()),
            "family": Counter(r["family"] for r in rows).most_common(),
            "mediation": Counter(r["mediation"] for r in rows).most_common(),
            "risk": Counter(r["risk"] for r in rows).most_common(),
            "behavior": Counter(r["behavior"] for r in rows).most_common(12),
            "evidence": Counter(r["evidenceSourceKind"] for r in rows).most_common(),
            "historyShape": Counter(r["historyShape"] for r in rows).most_common(10),
            "surfaceKind": Counter(r["surfaceKind"] for r in rows).most_common(),
            "flags": flags.most_common(),
            "wildLike": len(wild),
            "wildShare": (len(wild) / total) if total else 0.0,
            "structuredWild": len(structured),
            "structuredWildShare": (len(structured) / len(wild)) if wild else 0.0,
            "unresolvedWild": len(unresolved),
            "topRiskFiles": Counter(
                os.path.basename(r["file"])
                for r in rows
                if r["risk"] in {"High", "DangerousReinterpretation", "DangerousProvenance"}
            ).most_common(10),
        }

    def operations(self) -> dict[str, Any]:
        subjects = list(self.subjects.values())
        total = len(subjects)
        ops: Counter[str] = Counter()
        kinds: Counter[str] = Counter()
        for subject in subjects:
            operations = subject.get("operations") or {}
            for key, value in operations.items():
                if value is True:
                    ops[key] += 1
            kinds.update(operations.get("operationKinds") or [])
        declared = Counter(
            (s.get("declaredType") or {}).get("outerPointerKind", "unknown")
            for s in subjects
        )
        with_history = sum(1 for s in subjects if s.get("hasFinalHistory"))
        return {
            "subjects": total,
            "subjectsRaw": self.subject_rows_raw,
            "duplicationFactor": (self.subject_rows_raw / total) if total else 0.0,
            "withFinalHistory": with_history,
            "historyShare": (with_history / total) if total else 0.0,
            "primaryCategory": Counter(
                str(s.get("primaryCategory", "Unknown")) for s in subjects
            ).most_common(),
            "outerPointerKind": declared.most_common(),
            "operations": ops.most_common(16),
            "operationKinds": kinds.most_common(14),
            "callbackRecords": self.callback_records,
            "coverage": dict(sorted(self.coverage.items())),
        }

    def topology(self) -> dict[str, Any]:
        index = self.index
        if not index:
            return {}
        param = index.get("solvedParamSummaryFixpoint") or {}
        pointer_use = index.get("solvedPointerUseSummaryFixpoint") or {}
        edge = pointer_use.get("edgeResolution") or {}
        resolved = edge.get("numResolvedEdges", 0)
        inputs = edge.get("numInputEdges", 0)
        skipped_reasons = Counter()
        for entry in edge.get("skippedEdges") or []:
            reason = str(entry.get("reason", "unknown"))
            # "unresolved endpoint memcpy::fn:(...)" -> "unresolved endpoint memcpy"
            skipped_reasons[reason.split("::", 1)[0]] += 1
        return {
            "translationUnits": index.get("numSummaryFiles", 0),
            "functions": index.get("numFunctions", 0),
            "dependencyEdges": index.get("numSummaryDependencyEdges", 0),
            "paramTransferEdges": index.get("numSummaryParamTransferEdges", 0),
            "branchFamilies": index.get("numBranchFamilies", 0),
            "solvedParamFunctions": index.get(
                "numMaterializedSolvedParamSummaryFunctions", 0
            ),
            "paramConverged": param.get("converged"),
            "paramIterations": param.get("iterations"),
            "paramStoppedByTimeLimit": param.get("stoppedByTimeLimit"),
            "recursiveSCCFunctions": param.get("recursiveSCCFunctions", 0),
            "pointerUseConverged": pointer_use.get("converged"),
            "pointerUseRecords": pointer_use.get("numRecords", 0),
            "edgesInput": inputs,
            "edgesResolved": resolved,
            "edgesSkipped": edge.get("numSkippedEdges", 0),
            "edgeResolutionRate": (resolved / inputs) if inputs else 0.0,
            "topSkippedReasons": skipped_reasons.most_common(8),
            "aliasReturningFunctions": (
                index.get("solvedReturnSummaryFixpoint") or {}
            ).get("numAliasReturningFunctions", 0),
        }

    def cost(self) -> dict[str, Any]:
        total = self.runtime_ms.get("totalElapsed") or self.runtime_ms.get(
            "totalElapsedMilliseconds", 0
        )
        return {
            "translationUnits": self.runtime_tus,
            "totalMs": total,
            "phases": self.runtime_ms.most_common(12),
            "slowestTU": self.slowest_tu,
        }

    def health(self, known_families: set[str], known_mediations: set[str]) -> dict[str, Any]:
        checks: list[dict[str, str]] = []

        def add(level: str, title: str, detail: str) -> None:
            checks.append({"level": level, "title": title, "detail": detail})

        pointers = self.pointer_population()
        if self.prevalence_tus and pointers["declarations"] == 0:
            add(
                "critical",
                "Prevalence inventory is empty",
                "Every prevalence sidecar reported zero declarations and zero casts. "
                "The analyzer keeps only rows whose file path starts with "
                "--prevalence-metrics-project-root, and clang reports the path exactly "
                "as it appears in compile_commands.json. A relative source path in the "
                "'arguments' array therefore discards the whole inventory. Rebuild the "
                "compilation database with absolute source paths and re-run.",
            )
        inventory = self.coverage.get("declaredPointerInventoryRows")
        subject_rows = self.coverage.get("subjectRows", 0)
        if self.paper_facts_seen and subject_rows and inventory == 0:
            add(
                "critical",
                "Declared-pointer inventory is empty",
                "Paper-facts sidecars carry subject rows but no declared-pointer "
                "inventory rows. The inventory is scoped to "
                "--paper-facts-project-root by the same project-root prefix test "
                "the prevalence sidecar uses, so a relative source path in the "
                "compilation database empties it too. Rebuild the compilation "
                "database with absolute source paths and re-run.",
            )

        if not self.prevalence_tus:
            add(
                "warning",
                "No prevalence sidecars",
                "Pointer and cast distribution is unavailable. Re-run the runner with "
                "PREVALENCE_METRICS=1 (and PREVALENCE_INCLUDE_RETURN_TYPES=1 to count "
                "pointer-typed return definitions).",
            )

        if not self.reports_seen:
            add(
                "warning",
                "No classification sidecars",
                "stableClassification is only published in *.cast-report.json. Re-run "
                "with SUMMARY_REPORTS=1 to get the family/mediation/risk layer.",
            )
        if not self.paper_facts_seen:
            add(
                "warning",
                "No paper-facts sidecars",
                "Operation characteristics and declared-pointer kinds are unavailable. "
                "Re-run with PAPER_FACTS=1.",
            )

        if self.partial_tus or self.partial_sidecar_count:
            names = ", ".join(
                t["stem"] or os.path.basename(t["sourceFile"]) for t in self.partial_tus[:5]
            )
            more = "" if len(self.partial_tus) <= 5 else f" (+{len(self.partial_tus) - 5} more)"
            add(
                "warning",
                "Translation units failed during C1",
                f"{len(self.partial_tus) or 'Some'} translation unit(s) aborted in C1 "
                f"and are absent from the project index, so calls into them stay "
                f"unresolved and their declarations are missing from every "
                f"distribution here: {names}{more}. Their "
                f"{self.partial_sidecar_count} partial sidecar(s) under partial-*/ "
                "describe an incomplete analysis and are excluded from this profile "
                "(pass --include-partial to count them anyway).",
            )

        if self.key_collisions:
            add(
                "info",
                "Conflicting rows at the same source location",
                f"{self.key_collisions} record(s) share a project and source location "
                "with an earlier, different record. This happens for declarations and "
                "casts inside macros or conditionally compiled headers, which differ "
                "between translation units. The first row in sorted-path order is kept.",
            )

        if len(self.projects_seen) > 1:
            add(
                "info",
                "Artifacts from several projects under one root",
                "Sidecars naming "
                + str(len(self.projects_seen))
                + " different projects were read as one profile ("
                + ", ".join(sorted(self.projects_seen)[:6])
                + "). Records are keyed by project plus source location, so the "
                "projects are not conflated, but the totals are a sum over all of "
                "them. Use --mode corpus for a per-project breakdown.",
            )

        corrupt = {
            i.rsplit(": ", 1)[-1]
            for i in self.issues
            if i.startswith("non-UTF-8 bytes")
        }
        if corrupt:
            add(
                "serious",
                "Corrupt string fields in sidecars",
                f"{len(corrupt)} file(s) contain raw uninitialized bytes in a string "
                "field (tu.sourceFile). The analyzer binds a temporary std::string to a "
                "non-owning llvm::json::Value, so the value dangles before the file is "
                "written. Affected files were decoded with replacement.",
            )

        families = {f for f, _ in self.classification().get("family", [])}
        drift_f = families - known_families if known_families else set()
        risks = {r for r, _ in self.classification().get("risk", [])}
        drift_r = risks - KNOWN_RISKS
        if drift_f or drift_r:
            parts = []
            if drift_f:
                parts.append("family: " + ", ".join(sorted(drift_f)))
            if drift_r:
                parts.append("risk: " + ", ".join(sorted(drift_r)))
            add(
                "warning",
                "Classification vocabulary drift",
                "Labels emitted by the analyzer are absent from "
                "schemas/stable-classification.schema.json (" + "; ".join(parts) + "). "
                "Refresh the schema's x-c6f-known-values before running "
                "validate_classification_labels.py --strict-vocabulary.",
            )

        topology = self.topology()
        if topology:
            if topology.get("paramConverged") is False:
                add("serious", "Parameter fixpoint did not converge",
                    "Raise SUMMARY_IMPORT_ROUNDS or the solver iteration bound; "
                    "imported facts are incomplete.")
            if topology.get("paramStoppedByTimeLimit"):
                add("serious", "Solver stopped on its time limit",
                    "Results are a partial fixpoint.")
            rate = topology.get("edgeResolutionRate", 1.0)
            if topology.get("edgesInput") and rate < 0.8:
                add(
                    "warning",
                    "Low call-edge resolution",
                    f"{topology['edgesResolved']}/{topology['edgesInput']} "
                    f"({rate:.0%}) pointer-use transfer edges resolved. Unresolved "
                    "endpoints are usually libc functions with no analyzed body, which "
                    "caps how much cross-TU evidence the classification can use.",
                )

        if pointers["duplicationFactor"] > 1.5:
            add(
                "info",
                "Header declarations are shared across translation units",
                f"Summing per-TU counters would report "
                f"{pointers['tuSummed']} pointer declarations for "
                f"{pointers['declarations']} distinct ones "
                f"({pointers['duplicationFactor']:.1f}x). Project-level figures in this "
                "report are deduplicated by source location.",
            )

        if not checks:
            add("good", "No problems detected", "All expected artifact families present.")
        return {"checks": checks, "issues": self.issues}


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------


def build_profile(name: str, root: Path, args: argparse.Namespace) -> ProjectProfile:
    profile = ProjectProfile(name, root)
    found = collect_artifacts(
        root,
        include_intermediates=args.include_intermediates,
        include_partial=args.include_partial,
    )
    profile.artifacts = found

    limit_bytes = int(args.max_file_mb * 1024 * 1024) if args.max_file_mb else 0

    def ingest(paths: list[Path], handler, label: str) -> None:
        for path in paths:
            if limit_bytes:
                try:
                    if path.stat().st_size > limit_bytes:
                        profile.issues.append(
                            f"skipped {label} {path.name}: larger than "
                            f"{args.max_file_mb} MB (raise --max-file-mb)"
                        )
                        continue
                except OSError:
                    pass
            doc = load_json(path, profile.issues)
            if doc is not None:
                handler(doc)

    ingest(found.prevalence, profile.add_prevalence, "prevalence sidecar")
    if not args.no_classification:
        ingest(found.reports, profile.add_report, "report")
    ingest(found.paper_facts, profile.add_paper_facts, "paper-facts sidecar")
    ingest(found.link_summaries, profile.add_link_summary, "link summary")
    ingest(found.runtime, profile.add_runtime, "runtime sidecar")
    if found.index:
        doc = load_json(found.index, profile.issues)
        if doc is not None:
            profile.add_index(doc)
    for status_path in found.partial_status:
        doc = load_json(status_path, profile.issues)
        if doc is not None:
            profile.note_partial(doc, len(found.partial_sidecars))
    if found.partial_sidecars and not found.partial_status:
        profile.partial_sidecar_count = len(found.partial_sidecars)
    if args.project_root:
        profile.source_root = normalize_path(args.project_root)
    return profile


def assemble(profile: ProjectProfile, schema_dir: Path) -> dict[str, Any]:
    known_families = _schema_known_values(schema_dir, "family")
    known_mediations = _schema_known_values(schema_dir, "mediation")
    return {
        "schemaName": "cast-history-project-profile",
        "schemaVersion": 1,
        "generatedAt": _dt.datetime.now(_dt.timezone.utc)
        .replace(microsecond=0)
        .isoformat(),
        "project": profile.name,
        "resultRoot": str(profile.root),
        "sourceRoot": profile.source_root,
        "toolVersions": sorted(v for v in profile.tool_versions if v),
        "translationUnits": len(profile.tus),
        "artifactFamilies": profile.artifacts.families(),
        "pointerPopulation": profile.pointer_population(),
        "castDistribution": profile.cast_distribution(),
        "classification": profile.classification(),
        "operations": profile.operations(),
        "topology": profile.topology(),
        "cost": profile.cost(),
        "health": profile.health(known_families, known_mediations),
        "failedTranslationUnits": profile.partial_tus,
        "partialSidecars": profile.partial_sidecar_count,
    }


# --------------------------------------------------------------------------
# Rendering: terminal
# --------------------------------------------------------------------------

BAR_CHARS = "▏▎▍▌▋▊▉█"


def bar(value: float, peak: float, width: int = 22) -> str:
    if peak <= 0:
        return ""
    filled = (value / peak) * width
    full = int(filled)
    rest = filled - full
    out = "█" * full
    if full < width and rest > 0.06:
        out += BAR_CHARS[min(len(BAR_CHARS) - 1, int(rest * len(BAR_CHARS)))]
    return out


def pct(part: float, whole: float) -> str:
    return f"{(part / whole * 100):.1f}%" if whole else "n/a"


def dist_lines(pairs: list, width: int = 22, limit: int = 8, indent: str = "    ") -> list[str]:
    pairs = list(pairs)[:limit]
    if not pairs:
        return [indent + "(none)"]
    peak = max(v for _, v in pairs) if pairs else 0
    label_width = min(44, max(len(str(k)) for k, _ in pairs))
    return [
        f"{indent}{str(k)[:label_width]:<{label_width}}  {v:>6}  {bar(v, peak, width)}"
        for k, v in pairs
    ]


LEVEL_MARK = {
    "critical": "!!",
    "serious": "!!",
    "warning": " !",
    "info": " i",
    "good": " +",
}


def render_terminal(data: dict[str, Any]) -> str:
    out: list[str] = []
    w = 78
    ptr = data["pointerPopulation"]
    cast = data["castDistribution"]
    cls = data["classification"]
    ops = data["operations"]
    topo = data["topology"]
    cost = data["cost"]

    out.append("=" * w)
    out.append(f"  CAST-HISTORY PROJECT PROFILE   {data['project']}")
    out.append("=" * w)
    tools = ", ".join(data["toolVersions"]) or "unknown"
    out.append(
        f"  {data['translationUnits']} translation units"
        f"   ·   tool {tools}"
        f"   ·   {data['generatedAt']}"
    )
    present = [k for k, v in data["artifactFamilies"].items() if v]
    out.append(f"  artifacts: {', '.join(present) if present else 'none found'}")
    out.append("")

    # Headline numbers.
    out.append("  AT A GLANCE")
    out.append(
        f"    {ptr['declarations']:>7}  pointer declarations"
        f"   ({ptr['voidPointers']} void*, {pct(ptr['voidPointers'], ptr['declarations'])})"
    )
    out.append(
        f"    {cast['casts']:>7}  casts"
        f"   ({cast['explicit']} explicit, {pct(cast['explicit'], cast['casts'])})"
    )
    out.append(
        f"    {cls['rows']:>7}  classified subjects"
        f"   ({cls['wildLike']} surface-wild, {pct(cls['wildLike'], cls['rows'])})"
    )
    if cls["wildLike"]:
        out.append(
            f"    {cls['structuredWild']:>7}  of those resolve to a structured family"
            f"   ({pct(cls['structuredWild'], cls['wildLike'])})"
        )
    out.append(
        f"    {cast['toVoidPointer']:>7}  to-void casts"
        f"   ·   {cast['fromVoidPointer']} from-void"
        f"   ·   {cast['bytePointerCast']} byte-pointer"
    )
    if topo:
        out.append(
            f"    {topo['functions']:>7}  functions"
            f"   ·   {topo['dependencyEdges']} dependency edges"
            f"   ·   {topo['edgeResolutionRate']:.0%} edges resolved"
        )
    if cost.get("totalMs"):
        out.append(
            f"    {cost['totalMs'] / 1000:>7.1f}s analysis time over "
            f"{cost['translationUnits']} TUs"
        )
    out.append("")

    out.append("  POINTER POPULATION (deduplicated by declaration site)")
    out.extend(dist_lines(list(ptr["byKind"].items())))
    out.append(
        f"      void* {ptr['voidPointers']}  ·  function pointers "
        f"{ptr['functionPointers']}  ·  pointer fields {ptr['pointerFields']}"
        f"  ·  void* nested in function types {ptr['functionTypeVoidPositions']}"
    )
    if ptr["voidByKind"]:
        out.append(
            "      void* by kind: "
            + ", ".join(f"{k} {v}" for k, v in ptr["voidByKind"].items())
        )
    out.append("")
    out.append("  MOST DECLARED POINTER TYPES")
    out.extend(dist_lines(ptr["topTypes"], limit=8))
    out.append("")

    out.append("  CAST DISTRIBUTION")
    out.extend(dist_lines(list(cast["byKind"].items()), limit=6))
    out.append(
        f"      pointer→pointer {cast['pointerToPointer']}"
        f"  ·  to void* {cast['toVoidPointer']}"
        f"  ·  from void* {cast['fromVoidPointer']}"
        f"  ·  byte-pointer {cast['bytePointerCast']}"
    )
    out.append("")
    out.append("  MOST FREQUENT EXPLICIT CAST PAIRS")
    out.extend(dist_lines(cast["topExplicitPairs"], limit=8))
    out.append("")

    if cls["rows"]:
        out.append("  SEMANTIC FAMILY")
        out.extend(dist_lines(cls["family"], limit=10))
        out.append("")
        out.append("  MEDIATION KIND")
        out.extend(dist_lines(cls["mediation"], limit=8))
        out.append("")
        out.append("  RISK TIER")
        out.extend(dist_lines(cls["risk"], limit=8))
        out.append("")
        out.append("  EVIDENCE SOURCE")
        out.extend(dist_lines(cls["evidence"], limit=8))
        out.append("")

    if ops["subjects"]:
        out.append("  OPERATION CHARACTERISTICS")
        out.extend(dist_lines(ops["operations"], limit=9))
        out.append("")

    checks = data["health"]["checks"]
    out.append("  ANALYSIS HEALTH")
    for check in checks:
        mark = LEVEL_MARK.get(check["level"], " ·")
        out.append(f"   {mark} {check['title']}")
        for line in wrap(check["detail"], w - 8):
            out.append(f"        {line}")
    out.append("=" * w)
    return "\n".join(out)


def wrap(text: str, width: int) -> list[str]:
    words = text.split()
    lines: list[str] = []
    current: list[str] = []
    length = 0
    for word in words:
        if current and length + 1 + len(word) > width:
            lines.append(" ".join(current))
            current, length = [word], len(word)
        else:
            current.append(word)
            length += (1 if length else 0) + len(word)
    if current:
        lines.append(" ".join(current))
    return lines


# --------------------------------------------------------------------------
# Rendering: Markdown
# --------------------------------------------------------------------------


def md_table(headers: list[str], rows: list[list[Any]]) -> list[str]:
    out = ["| " + " | ".join(headers) + " |",
           "|" + "|".join("---" for _ in headers) + "|"]
    for row in rows:
        out.append("| " + " | ".join(str(c) for c in row) + " |")
    return out


def md_dist(pairs: list, total: int, label: str, limit: int = 20) -> list[str]:
    rows = [[k, v, pct(v, total)] for k, v in list(pairs)[:limit]]
    return md_table([label, "count", "share"], rows)


def render_markdown(data: dict[str, Any]) -> str:
    ptr = data["pointerPopulation"]
    cast = data["castDistribution"]
    cls = data["classification"]
    ops = data["operations"]
    topo = data["topology"]
    cost = data["cost"]
    out: list[str] = []
    a = out.append

    a(f"# Cast-history profile: {data['project']}")
    a("")
    a(f"Generated {data['generatedAt']} from `{data['resultRoot']}`.")
    a("")
    a("## Summary")
    a("")
    out.extend(md_table(
        ["measure", "value"],
        [
            ["Translation units", data["translationUnits"]],
            ["Pointer declarations (distinct sites)", ptr["declarations"]],
            ["void* declarations", f"{ptr['voidPointers']} ({pct(ptr['voidPointers'], ptr['declarations'])})"],
            ["Function-pointer declarations", ptr["functionPointers"]],
            ["Pointer fields", ptr["pointerFields"]],
            ["Casts (distinct sites)", cast["casts"]],
            ["Explicit casts", f"{cast['explicit']} ({pct(cast['explicit'], cast['casts'])})"],
            ["to-void / from-void / byte casts",
             f"{cast['toVoidPointer']} / {cast['fromVoidPointer']} / {cast['bytePointerCast']}"],
            ["Classified subjects", cls["rows"]],
            ["Surface-wild subjects", f"{cls['wildLike']} ({pct(cls['wildLike'], cls['rows'])})"],
            ["Surface-wild resolving to a structured family",
             f"{cls['structuredWild']} ({pct(cls['structuredWild'], cls['wildLike'])})"],
            ["Paper-facts subjects", ops["subjects"]],
            ["Subjects with a final history",
             f"{ops['withFinalHistory']} ({pct(ops['withFinalHistory'], ops['subjects'])})"],
        ],
    ))
    a("")

    a("## Analysis health")
    a("")
    for check in data["health"]["checks"]:
        a(f"- **[{check['level']}] {check['title']}** — {check['detail']}")
    a("")

    a("## Pointer population")
    a("")
    a(f"Counts are deduplicated by declaration site. Summing the per-TU counters "
      f"instead would report {ptr['tuSummed']} declarations "
      f"({ptr['duplicationFactor']:.1f}x) because declarations in shared headers "
      f"are re-counted by every including translation unit.")
    a("")
    out.extend(md_dist(list(ptr["byKind"].items()), ptr["declarations"], "declaration kind"))
    a("")
    a("### Most declared pointer types")
    a("")
    out.extend(md_dist(ptr["topTypes"], ptr["declarations"], "canonical type", limit=15))
    a("")
    a("### Declaration sites by file")
    a("")
    out.extend(md_dist(ptr["topFiles"], ptr["declarations"], "file", limit=15))
    a("")

    a("## Cast distribution")
    a("")
    out.extend(md_dist(list(cast["byKind"].items()), cast["casts"], "cast kind"))
    a("")
    out.extend(md_table(
        ["boundary", "count", "share of casts"],
        [
            ["pointer -> pointer", cast["pointerToPointer"], pct(cast["pointerToPointer"], cast["casts"])],
            ["to void*", cast["toVoidPointer"], pct(cast["toVoidPointer"], cast["casts"])],
            ["from void*", cast["fromVoidPointer"], pct(cast["fromVoidPointer"], cast["casts"])],
            ["byte-pointer", cast["bytePointerCast"], pct(cast["bytePointerCast"], cast["casts"])],
        ],
    ))
    a("")
    a("### Most frequent explicit cast pairs")
    a("")
    out.extend(md_dist(cast["topExplicitPairs"], cast["explicit"], "source -> destination", limit=15))
    a("")

    if cls["rows"]:
        a("## Stable classification")
        a("")
        a(f"{cls['rows']} distinct subjects "
          f"({cls['projectOwned']} declared inside the project root, "
          f"{cls['external']} in system or library headers). "
          f"Per-TU rows before deduplication: {cls['rowsRaw']} "
          f"({cls['duplicationFactor']:.1f}x).")
        a("")
        for title, key in [
            ("Semantic family", "family"),
            ("Mediation kind", "mediation"),
            ("Risk tier", "risk"),
            ("Behavior", "behavior"),
            ("Evidence source", "evidence"),
            ("History shape", "historyShape"),
            ("Surface kind", "surfaceKind"),
        ]:
            a(f"### {title}")
            a("")
            out.extend(md_dist(cls[key], cls["rows"], title.lower()))
            a("")
        if cls["flags"]:
            a("### Mediation flags")
            a("")
            out.extend(md_dist(cls["flags"], cls["rows"], "flag"))
            a("")

    if ops["subjects"]:
        a("## Operation characteristics")
        a("")
        out.extend(md_dist(ops["primaryCategory"], ops["subjects"], "primary category"))
        a("")
        a("### Declared outer pointer kind")
        a("")
        out.extend(md_dist(ops["outerPointerKind"], ops["subjects"], "outer pointer kind"))
        a("")
        a("### Operations observed on subjects")
        a("")
        out.extend(md_dist(ops["operations"], ops["subjects"], "operation"))
        a("")
        a("### Pointer-use operation kinds")
        a("")
        out.extend(md_dist(ops["operationKinds"], ops["subjects"], "operation kind"))
        a("")

    if topo:
        a("## Cross-TU topology and solver")
        a("")
        out.extend(md_table(
            ["measure", "value"],
            [
                ["Functions in the project index", topo["functions"]],
                ["Summary dependency edges", topo["dependencyEdges"]],
                ["Parameter transfer edges", topo["paramTransferEdges"]],
                ["Functions with a solved parameter summary", topo["solvedParamFunctions"]],
                ["Alias-returning functions", topo["aliasReturningFunctions"]],
                ["Recursive SCC functions", topo["recursiveSCCFunctions"]],
                ["Parameter fixpoint converged", topo["paramConverged"]],
                ["Parameter fixpoint iterations", topo["paramIterations"]],
                ["Pointer-use fixpoint converged", topo["pointerUseConverged"]],
                ["Pointer-use records", topo["pointerUseRecords"]],
                ["Call-edge resolution",
                 f"{topo['edgesResolved']}/{topo['edgesInput']} ({topo['edgeResolutionRate']:.0%})"],
            ],
        ))
        a("")
        if topo["topSkippedReasons"]:
            a("### Unresolved call edges")
            a("")
            out.extend(md_dist(topo["topSkippedReasons"], topo["edgesSkipped"], "reason"))
            a("")

    if cost.get("totalMs"):
        a("## Analysis cost")
        a("")
        out.extend(md_dist(cost["phases"], cost["totalMs"], "phase (ms)"))
        a("")
        if cost.get("slowestTU"):
            a(f"Slowest translation unit: `{cost['slowestTU'][0]}` "
              f"at {cost['slowestTU'][1]} ms.")
            a("")

    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# Rendering: CSV tables
# --------------------------------------------------------------------------


def write_tables(data: dict[str, Any], out_dir: Path) -> list[Path]:
    tables: dict[str, tuple[list[str], list[list[Any]]]] = {}
    ptr = data["pointerPopulation"]
    cast = data["castDistribution"]
    cls = data["classification"]
    ops = data["operations"]

    def dist(pairs: list, total: int, label: str) -> tuple[list[str], list[list[Any]]]:
        return ([label, "count", "share"],
                [[k, v, round(v / total, 6) if total else 0] for k, v in pairs])

    tables["pointer-declaration-kind"] = dist(
        list(ptr["byKind"].items()), ptr["declarations"], "declarationKind")
    tables["pointer-declared-type"] = dist(
        ptr["topTypes"], ptr["declarations"], "canonicalType")
    tables["pointer-declaration-file"] = dist(
        ptr["topFiles"], ptr["declarations"], "file")
    tables["cast-kind"] = dist(list(cast["byKind"].items()), cast["casts"], "castKind")
    tables["cast-explicit-pairs"] = dist(
        cast["topExplicitPairs"], cast["explicit"], "castPair")
    tables["cast-void-boundary"] = (
        ["boundary", "count", "share"],
        [[k, cast[v], round(cast[v] / cast["casts"], 6) if cast["casts"] else 0]
         for k, v in [("toVoidPointer", "toVoidPointer"),
                      ("fromVoidPointer", "fromVoidPointer"),
                      ("bytePointerCast", "bytePointerCast"),
                      ("pointerToPointer", "pointerToPointer")]],
    )
    if cls["rows"]:
        for name, key in [
            ("classification-family", "family"),
            ("classification-mediation", "mediation"),
            ("classification-risk", "risk"),
            ("classification-behavior", "behavior"),
            ("classification-evidence", "evidence"),
            ("classification-history-shape", "historyShape"),
            ("classification-surface-kind", "surfaceKind"),
            ("classification-flags", "flags"),
        ]:
            tables[name] = dist(cls[key], cls["rows"], key)
    if ops["subjects"]:
        tables["operations-primary-category"] = dist(
            ops["primaryCategory"], ops["subjects"], "primaryCategory")
        tables["operations-outer-pointer-kind"] = dist(
            ops["outerPointerKind"], ops["subjects"], "outerPointerKind")
        tables["operations-observed"] = dist(
            ops["operations"], ops["subjects"], "operation")
        tables["operations-kinds"] = dist(
            ops["operationKinds"], ops["subjects"], "operationKind")

    out_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []
    for name, (headers, rows) in tables.items():
        path = out_dir / f"{name}.csv"
        with path.open("w", newline="", encoding="utf-8") as fh:
            writer = csv.writer(fh)
            writer.writerow(headers)
            writer.writerows(rows)
        written.append(path)
    return written


# --------------------------------------------------------------------------
# Rendering: HTML dashboard
# --------------------------------------------------------------------------

HTML_HEAD = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>__TITLE__</title>
<style>
  :root {
    color-scheme: light;
    --page:           #f9f9f7;
    --surface-1:      #fcfcfb;
    --text-primary:   #0b0b0b;
    --text-secondary: #52514e;
    --text-muted:     #898781;
    --grid:           #e1e0d9;
    --baseline:       #c3c2b7;
    --border:         rgba(11,11,11,0.10);
    --series-1:       #2a78d6;
    --series-2:       #eb6834;
    --series-3:       #1baf7a;
    --good:           #0ca30c;
    --warning:        #fab219;
    --serious:        #ec835a;
    --critical:       #d03b3b;
  }
  @media (prefers-color-scheme: dark) {
    :root:where(:not([data-theme="light"])) {
      color-scheme: dark;
      --page:           #0d0d0d;
      --surface-1:      #1a1a19;
      --text-primary:   #f0efec;
      --text-secondary: #c3c2b7;
      --text-muted:     #898781;
      --grid:           #2c2c2a;
      --baseline:       #383835;
      --border:         rgba(255,255,255,0.10);
      --series-1:       #3987e5;
      --series-2:       #d95926;
      --series-3:       #199e70;
    }
  }
  :root[data-theme="dark"] {
    color-scheme: dark;
    --page:           #0d0d0d;
    --surface-1:      #1a1a19;
    --text-primary:   #f0efec;
    --text-secondary: #c3c2b7;
    --text-muted:     #898781;
    --grid:           #2c2c2a;
    --baseline:       #383835;
    --border:         rgba(255,255,255,0.10);
    --series-1:       #3987e5;
    --series-2:       #d95926;
    --series-3:       #199e70;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 0 16px 64px;
    background: var(--page); color: var(--text-primary);
    font: 15px/1.55 ui-sans-serif, system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    -webkit-font-smoothing: antialiased;
  }
  .wrap { max-width: 1120px; margin: 0 auto; }
  header { padding: 40px 0 8px; }
  h1 { font-size: 1.6rem; line-height: 1.2; margin: 0 0 6px; letter-spacing: -0.01em; }
  .sub { color: var(--text-secondary); font-size: 0.875rem; margin: 0; }
  .sub code { font-size: 0.8125rem; word-break: break-all; }
  h2 {
    font-size: 0.8125rem; text-transform: uppercase; letter-spacing: 0.07em;
    color: var(--text-muted); font-weight: 600;
    margin: 40px 0 12px; padding-bottom: 6px; border-bottom: 1px solid var(--grid);
  }
  h3 { font-size: 0.9375rem; margin: 0 0 10px; font-weight: 600; }
  .tiles { display: grid; gap: 10px; grid-template-columns: repeat(auto-fit, minmax(170px, 1fr)); }
  .tile {
    background: var(--surface-1); border: 1px solid var(--border);
    border-radius: 10px; padding: 14px 16px;
  }
  .tile .v { font-size: 1.75rem; font-weight: 650; letter-spacing: -0.02em; font-variant-numeric: tabular-nums; }
  .tile .k { font-size: 0.8125rem; color: var(--text-secondary); margin-top: 2px; }
  .tile .n { font-size: 0.75rem; color: var(--text-muted); margin-top: 4px; }
  .cards { display: grid; gap: 14px; align-items: start;
           grid-template-columns: repeat(auto-fit, minmax(330px, 1fr)); }
  .card {
    background: var(--surface-1); border: 1px solid var(--border);
    border-radius: 10px; padding: 16px 18px 18px;
  }
  .card.wide { grid-column: 1 / -1; }
  table.chart { width: 100%; border-collapse: collapse; }
  table.chart td { padding: 3px 0; vertical-align: middle; }
  table.chart td.lbl {
    font-size: 0.8125rem; color: var(--text-secondary);
    padding-right: 10px; white-space: nowrap; max-width: 230px;
    overflow: hidden; text-overflow: ellipsis;
  }
  table.chart td.lbl code { font-size: 0.78125rem; }
  table.chart td.track { width: 100%; padding: 3px 12px; }
  .track-bg { background: var(--grid); border-radius: 4px; height: 13px; width: 100%; }
  .track-bg > i {
    display: block; height: 13px; border-radius: 4px;
    background: var(--series-1); min-width: 2px;
  }
  table.chart td.val {
    font-size: 0.8125rem; font-variant-numeric: tabular-nums;
    text-align: right; padding-left: 10px; white-space: nowrap;
    color: var(--text-primary);
  }
  table.chart td.val .pc { color: var(--text-muted); font-size: 0.75rem; margin-left: 5px; }
  table.chart tr:hover td.lbl, table.chart tr:hover td.val { color: var(--text-primary); }
  .checks { display: grid; gap: 8px; }
  .check {
    display: grid; grid-template-columns: 22px 1fr; gap: 10px;
    background: var(--surface-1); border: 1px solid var(--border);
    border-left-width: 3px; border-radius: 8px; padding: 11px 14px;
  }
  .check .ico { font-size: 0.9375rem; line-height: 1.5; }
  .check .t { font-weight: 600; font-size: 0.875rem; }
  .check .d { color: var(--text-secondary); font-size: 0.8125rem; margin-top: 3px; }
  .check.critical, .check.serious { border-left-color: var(--critical); }
  .check.warning { border-left-color: var(--warning); }
  .check.info    { border-left-color: var(--series-1); }
  .check.good    { border-left-color: var(--good); }
  table.grid { width: 100%; border-collapse: collapse; font-size: 0.8125rem; }
  table.grid th {
    text-align: left; font-weight: 600; color: var(--text-muted);
    font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.05em;
    padding: 5px 10px 5px 0; border-bottom: 1px solid var(--grid);
  }
  table.grid td { padding: 5px 10px 5px 0; border-bottom: 1px solid var(--grid); }
  table.grid td.num { text-align: right; font-variant-numeric: tabular-nums; padding-right: 0; }
  table.grid tr:last-child td { border-bottom: 0; }
  footer { color: var(--text-muted); font-size: 0.75rem; margin-top: 40px;
           padding-top: 14px; border-top: 1px solid var(--grid); }
  @media (max-width: 560px) {
    .tile .v { font-size: 1.4rem; }
    table.chart td.lbl { max-width: 140px; }
  }
</style>
</head>
<body>
<div class="wrap">
"""


def esc(value: Any) -> str:
    return (
        str(value)
        .replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def html_bars(pairs: list, total: int, *, limit: int = 12, mono: bool = False) -> str:
    pairs = list(pairs)[:limit]
    if not pairs:
        return '<p class="sub">No data.</p>'
    peak = max((v for _, v in pairs), default=0) or 1
    rows = []
    for key, value in pairs:
        width = max(1.0, value / peak * 100)
        label = f"<code>{esc(key)}</code>" if mono else esc(key)
        share = f'<span class="pc">{value / total * 100:.1f}%</span>' if total else ""
        rows.append(
            f'<tr title="{esc(key)}: {value}">'
            f'<td class="lbl">{label}</td>'
            f'<td class="track"><div class="track-bg">'
            f'<i style="width:{width:.2f}%"></i></div></td>'
            f'<td class="val">{value}{share}</td></tr>'
        )
    return '<table class="chart"><tbody>' + "".join(rows) + "</tbody></table>"


def html_card(title: str, body: str, *, wide: bool = False, note: str = "") -> str:
    klass = "card wide" if wide else "card"
    note_html = f'<p class="sub" style="margin:-4px 0 10px">{esc(note)}</p>' if note else ""
    return f'<div class="{klass}"><h3>{esc(title)}</h3>{note_html}{body}</div>'


def html_grid(headers: list[str], rows: list[list[Any]]) -> str:
    head = "".join(f"<th>{esc(h)}</th>" for h in headers)
    body = []
    for row in rows:
        cells = "".join(
            f'<td class="num">{esc(c)}</td>' if i else f"<td>{esc(c)}</td>"
            for i, c in enumerate(row)
        )
        body.append(f"<tr>{cells}</tr>")
    return (
        f'<table class="grid"><thead><tr>{head}</tr></thead>'
        f"<tbody>{''.join(body)}</tbody></table>"
    )


CHECK_ICON = {
    "critical": "✕", "serious": "✕", "warning": "▲", "info": "i", "good": "✓",
}


def render_html(data: dict[str, Any]) -> str:
    ptr = data["pointerPopulation"]
    cast = data["castDistribution"]
    cls = data["classification"]
    ops = data["operations"]
    topo = data["topology"]
    cost = data["cost"]
    title = f"Cast profile — {data['project']}"

    out = [HTML_HEAD.replace("__TITLE__", esc(title))]
    a = out.append

    a("<header>")
    a(f"<h1>{esc(data['project'])}</h1>")
    a(f'<p class="sub">{data["translationUnits"]} translation units · '
      f'tool {esc(", ".join(data["toolVersions"]) or "unknown")} · '
      f'generated {esc(data["generatedAt"])}</p>')
    a(f'<p class="sub"><code>{esc(data["resultRoot"])}</code></p>')
    a("</header>")

    # Headline tiles.
    tiles = [
        (f"{ptr['declarations']:,}", "pointer declarations",
         f"{ptr['voidPointers']} void* ({pct(ptr['voidPointers'], ptr['declarations'])})"),
        (f"{cast['casts']:,}", "cast sites",
         f"{cast['explicit']} explicit ({pct(cast['explicit'], cast['casts'])})"),
        (f"{cls['rows']:,}", "classified subjects",
         f"{cls['projectOwned']} inside the project"),
        (f"{pct(cls['wildLike'], cls['rows'])}", "surface-wild",
         f"{cls['structuredWild']} of {cls['wildLike']} resolve to a structured family"),
    ]
    if topo:
        tiles.append((f"{topo['functions']:,}", "functions in index",
                      f"{topo['edgeResolutionRate']:.0%} call edges resolved"))
    if cost.get("totalMs"):
        tiles.append((f"{cost['totalMs'] / 1000:.1f}s", "analysis time",
                      f"over {cost['translationUnits']} TUs"))
    a('<div class="tiles">')
    for value, key, note in tiles:
        a(f'<div class="tile"><div class="v">{esc(value)}</div>'
          f'<div class="k">{esc(key)}</div><div class="n">{esc(note)}</div></div>')
    a("</div>")

    # Health.
    a("<h2>Analysis health</h2>")
    a('<div class="checks">')
    for check in data["health"]["checks"]:
        level = check["level"]
        a(f'<div class="check {esc(level)}">'
          f'<div class="ico">{CHECK_ICON.get(level, "·")}</div>'
          f'<div><div class="t">{esc(check["title"])}</div>'
          f'<div class="d">{esc(check["detail"])}</div></div></div>')
    a("</div>")

    # Pointer population.
    a("<h2>Pointer population</h2>")
    a('<div class="cards">')
    a(html_card(
        "Declaration kind",
        html_bars(list(ptr["byKind"].items()), ptr["declarations"]),
        note=f"Deduplicated by declaration site; per-TU sum would be "
             f"{ptr['tuSummed']} ({ptr['duplicationFactor']:.1f}x).",
    ))
    a(html_card("Pointer shape", html_grid(
        ["kind", "count", "share"],
        [
            ["void* declarations", ptr["voidPointers"], pct(ptr["voidPointers"], ptr["declarations"])],
            ["function pointers", ptr["functionPointers"], pct(ptr["functionPointers"], ptr["declarations"])],
            ["pointer fields", ptr["pointerFields"], pct(ptr["pointerFields"], ptr["declarations"])],
            ["void* nested in function types", ptr["functionTypeVoidPositions"], "—"],
        ],
    )))
    a(html_card("Most declared pointer types",
                html_bars(ptr["topTypes"], ptr["declarations"], mono=True), wide=True))
    a(html_card("Declaration sites by file",
                html_bars(ptr["topFiles"], ptr["declarations"], mono=True)))
    if ptr["voidByKind"]:
        a(html_card("void* by declaration kind",
                    html_bars(list(ptr["voidByKind"].items()), ptr["voidPointers"])))
    a("</div>")

    # Casts.
    a("<h2>Cast distribution</h2>")
    a('<div class="cards">')
    a(html_card("Cast kind", html_bars(list(cast["byKind"].items()), cast["casts"])))
    a(html_card("Type-erasure boundaries", html_grid(
        ["boundary", "count", "share"],
        [
            ["pointer → pointer", cast["pointerToPointer"], pct(cast["pointerToPointer"], cast["casts"])],
            ["to void*", cast["toVoidPointer"], pct(cast["toVoidPointer"], cast["casts"])],
            ["from void*", cast["fromVoidPointer"], pct(cast["fromVoidPointer"], cast["casts"])],
            ["byte-pointer", cast["bytePointerCast"], pct(cast["bytePointerCast"], cast["casts"])],
        ],
    )))
    a(html_card("Most frequent explicit cast pairs",
                html_bars(cast["topExplicitPairs"], cast["explicit"], mono=True), wide=True))
    a(html_card("Explicit casts by file",
                html_bars(cast["topFiles"], cast["explicit"], mono=True)))
    a("</div>")

    # Classification.
    if cls["rows"]:
        a("<h2>Stable classification</h2>")
        a('<div class="cards">')
        a(html_card("Semantic family", html_bars(cls["family"], cls["rows"]), wide=True))
        a(html_card("Mediation kind", html_bars(cls["mediation"], cls["rows"])))
        a(html_card("Risk tier", html_bars(cls["risk"], cls["rows"])))
        a(html_card("Behavior", html_bars(cls["behavior"], cls["rows"])))
        a(html_card("Evidence source", html_bars(cls["evidence"], cls["rows"])))
        a(html_card("History shape", html_bars(cls["historyShape"], cls["rows"])))
        a(html_card("Surface kind", html_bars(cls["surfaceKind"], cls["rows"])))
        if cls["flags"]:
            a(html_card("Mediation flags", html_bars(cls["flags"], cls["rows"])))
        a("</div>")

    # Operations.
    if ops["subjects"]:
        a("<h2>Operation characteristics</h2>")
        a('<div class="cards">')
        a(html_card("Primary category", html_bars(ops["primaryCategory"], ops["subjects"]), wide=True))
        a(html_card("Declared outer pointer kind",
                    html_bars(ops["outerPointerKind"], ops["subjects"])))
        a(html_card("Operations observed", html_bars(ops["operations"], ops["subjects"], limit=14)))
        a(html_card("Pointer-use operation kinds",
                    html_bars(ops["operationKinds"], ops["subjects"], limit=14)))
        a("</div>")

    # Topology and cost.
    if topo or cost.get("totalMs"):
        a("<h2>Topology and cost</h2>")
        a('<div class="cards">')
        if topo:
            a(html_card("Project index", html_grid(
                ["measure", "value"],
                [
                    ["functions", topo["functions"]],
                    ["dependency edges", topo["dependencyEdges"]],
                    ["parameter transfer edges", topo["paramTransferEdges"]],
                    ["solved parameter summaries", topo["solvedParamFunctions"]],
                    ["alias-returning functions", topo["aliasReturningFunctions"]],
                    ["recursive SCC functions", topo["recursiveSCCFunctions"]],
                    ["parameter fixpoint converged", topo["paramConverged"]],
                    ["pointer-use fixpoint converged", topo["pointerUseConverged"]],
                    ["call edges resolved",
                     f"{topo['edgesResolved']}/{topo['edgesInput']} ({topo['edgeResolutionRate']:.0%})"],
                ],
            )))
            if topo["topSkippedReasons"]:
                a(html_card("Unresolved call edges",
                            html_bars(topo["topSkippedReasons"], topo["edgesSkipped"], mono=True)))
        if cost.get("totalMs"):
            a(html_card("Analysis time by phase",
                        html_bars(cost["phases"], cost["totalMs"]),
                        note="Milliseconds summed across translation units."))
        a("</div>")

    a(f'<footer>Generated by <code>scripts/summarize_project_cast_profile.py</code> '
      f'from cast-history runner output. Counts are deduplicated by source location '
      f'unless stated otherwise.</footer>')
    a("</div></body></html>")
    return "\n".join(out)


# --------------------------------------------------------------------------
# Corpus mode
# --------------------------------------------------------------------------

CORPUS_COLUMNS = [
    ("project", lambda d: d["project"]),
    ("tus", lambda d: d["translationUnits"]),
    ("pointer_decls", lambda d: d["pointerPopulation"]["declarations"]),
    ("void_ptr_decls", lambda d: d["pointerPopulation"]["voidPointers"]),
    ("void_ptr_share", lambda d: round(d["pointerPopulation"]["voidPointerShare"], 4)),
    ("fn_ptr_decls", lambda d: d["pointerPopulation"]["functionPointers"]),
    ("pointer_fields", lambda d: d["pointerPopulation"]["pointerFields"]),
    ("casts", lambda d: d["castDistribution"]["casts"]),
    ("explicit_casts", lambda d: d["castDistribution"]["explicit"]),
    ("to_void_casts", lambda d: d["castDistribution"]["toVoidPointer"]),
    ("from_void_casts", lambda d: d["castDistribution"]["fromVoidPointer"]),
    ("byte_casts", lambda d: d["castDistribution"]["bytePointerCast"]),
    ("classified_subjects", lambda d: d["classification"]["rows"]),
    ("wild_like", lambda d: d["classification"]["wildLike"]),
    ("wild_share", lambda d: round(d["classification"]["wildShare"], 4)),
    ("structured_wild", lambda d: d["classification"]["structuredWild"]),
    ("structured_wild_share", lambda d: round(d["classification"]["structuredWildShare"], 4)),
    ("functions", lambda d: d["topology"].get("functions", 0)),
    ("edge_resolution", lambda d: round(d["topology"].get("edgeResolutionRate", 0.0), 4)),
    ("analysis_ms", lambda d: d["cost"].get("totalMs", 0)),
    ("health_worst", lambda d: worst_level(d)),
]

LEVEL_ORDER = {"critical": 4, "serious": 3, "warning": 2, "info": 1, "good": 0}


def worst_level(data: dict[str, Any]) -> str:
    levels = [c["level"] for c in data["health"]["checks"]]
    if not levels:
        return "good"
    return max(levels, key=lambda l: LEVEL_ORDER.get(l, 0))


def render_corpus_terminal(profiles: list[dict[str, Any]]) -> str:
    out = ["=" * 78, f"  CAST-HISTORY CORPUS PROFILE   {len(profiles)} project(s)", "=" * 78]
    header = f"  {'project':<24}{'TUs':>5}{'ptrs':>8}{'void*':>7}{'casts':>8}{'subj':>7}{'wild':>7}  health"
    out.append(header)
    out.append("  " + "-" * 74)
    for data in sorted(profiles, key=lambda d: -d["pointerPopulation"]["declarations"]):
        out.append(
            f"  {data['project'][:23]:<24}"
            f"{data['translationUnits']:>5}"
            f"{data['pointerPopulation']['declarations']:>8}"
            f"{data['pointerPopulation']['voidPointers']:>7}"
            f"{data['castDistribution']['casts']:>8}"
            f"{data['classification']['rows']:>7}"
            f"{pct(data['classification']['wildLike'], data['classification']['rows']):>7}"
            f"  {LEVEL_MARK.get(worst_level(data), ' ·')} {worst_level(data)}"
        )
    totals = {
        "pointers": sum(d["pointerPopulation"]["declarations"] for d in profiles),
        "voids": sum(d["pointerPopulation"]["voidPointers"] for d in profiles),
        "casts": sum(d["castDistribution"]["casts"] for d in profiles),
        "subjects": sum(d["classification"]["rows"] for d in profiles),
        "wild": sum(d["classification"]["wildLike"] for d in profiles),
        "structured": sum(d["classification"]["structuredWild"] for d in profiles),
    }
    out.append("  " + "-" * 74)
    out.append(
        f"  {'TOTAL':<24}"
        f"{sum(d['translationUnits'] for d in profiles):>5}"
        f"{totals['pointers']:>8}{totals['voids']:>7}"
        f"{totals['casts']:>8}{totals['subjects']:>7}"
        f"{pct(totals['wild'], totals['subjects']):>7}"
    )
    out.append("")
    out.append("  CORPUS FAMILY DISTRIBUTION")
    families: Counter[str] = Counter()
    for data in profiles:
        for name, count in data["classification"]["family"]:
            families[name] += count
    out.extend(dist_lines(families.most_common(), limit=12))
    out.append("")
    out.append(
        f"  Surface-wild subjects resolving to a structured family: "
        f"{totals['structured']}/{totals['wild']} "
        f"({pct(totals['structured'], totals['wild'])})"
    )
    out.append("=" * 78)
    return "\n".join(out)


def write_corpus_csv(profiles: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow([name for name, _ in CORPUS_COLUMNS])
        for data in sorted(profiles, key=lambda d: d["project"]):
            writer.writerow([fn(data) for _, fn in CORPUS_COLUMNS])


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(
        description="Summarize cast-history runner output into a project profile.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    ap.add_argument("--root", type=Path, required=True,
                    help="a project result directory, or a root containing several")
    ap.add_argument("--out-dir", type=Path,
                    help="write cast-profile.{md,json,html} and tables/ here "
                         "(default: <root>/cast-profile)")
    ap.add_argument("--project-name", help="override the project name")
    ap.add_argument("--project-root",
                    help="source tree of the analyzed project; used to separate "
                         "project-owned subjects from system headers when the "
                         "prevalence sidecars do not record it")
    ap.add_argument("--no-classification", action="store_true",
                    help="skip *.cast-report.json (the largest sidecars) and report "
                         "only pointer/cast prevalence and paper-facts")
    ap.add_argument("--max-file-mb", type=float, default=0.0,
                    help="skip sidecars larger than this many MB (0 = no limit)")
    ap.add_argument("--include-intermediates", action="store_true",
                    help="also read per-round scratch directories (inflates counts)")
    ap.add_argument("--include-partial", action="store_true",
                    help="also count partial-*/ sidecars of translation units that "
                         "failed during C1; off by default because they describe an "
                         "aborted analysis and the runner never indexes them")
    ap.add_argument("--mode", choices=["auto", "project", "corpus"], default="auto",
                    help="treat --root as one project, as a corpus of projects, or "
                         "decide from its contents (default)")
    ap.add_argument("--strict", action="store_true",
                    help="exit non-zero on a serious finding as well as a critical one")
    ap.add_argument("--quiet", action="store_true", help="suppress the terminal report")
    ap.add_argument("--no-files", action="store_true",
                    help="terminal report only; write nothing to disk")
    return ap.parse_args(argv)


def discover_projects(root: Path, mode: str) -> tuple[list[Path], bool]:
    """Return the project directories to profile and whether this is a corpus."""
    kind = mode if mode != "auto" else classify_root(root)
    if kind == "project":
        return [root], False
    if kind == "corpus":
        projects = [
            child
            for child in subdirectories(root)
            if owns_artifacts(child)
            or any(owns_artifacts(gc) for gc in subdirectories(child))
        ]
        # A runner writes one project to <output>/<name>/ as well, so a root
        # holding a single project is reported as that project unless the
        # caller explicitly asked for the corpus table.
        if len(projects) == 1 and mode == "auto":
            return projects, False
        return projects, True
    return [], False


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()
    if not root.is_dir():
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2

    schema_dir = Path(__file__).resolve().parent.parent / "schemas"
    projects, corpus = discover_projects(root, args.mode)
    if not projects:
        print(
            f"error: no cast-history artifacts under {root}\n"
            "       Expected *.cast-summary.json, *.cast-report.json, "
            "*.cast-prevalence.json,\n"
            "       *.cast-paper-facts.json or project-summary-index.json.",
            file=sys.stderr,
        )
        return 1

    out_root = args.out_dir.resolve() if args.out_dir else root / "cast-profile"
    results: list[dict[str, Any]] = []

    for project_dir in projects:
        name = args.project_name if (args.project_name and not corpus) else project_dir.name
        profile = build_profile(name, project_dir, args)
        data = assemble(profile, schema_dir)
        results.append(data)

        if not args.no_files:
            dest = out_root / name if corpus else out_root
            dest.mkdir(parents=True, exist_ok=True)
            (dest / "cast-profile.json").write_text(
                json.dumps(data, indent=1, sort_keys=False) + "\n", encoding="utf-8")
            (dest / "cast-profile.md").write_text(render_markdown(data), encoding="utf-8")
            (dest / "cast-profile.html").write_text(render_html(data), encoding="utf-8")
            write_tables(data, dest / "tables")

    if corpus:
        if not args.quiet:
            print(render_corpus_terminal(results))
        if not args.no_files:
            out_root.mkdir(parents=True, exist_ok=True)
            write_corpus_csv(results, out_root / "corpus-overview.csv")
            (out_root / "corpus-overview.json").write_text(
                json.dumps(
                    {
                        "schemaName": "cast-history-corpus-profile",
                        "schemaVersion": 1,
                        "projects": results,
                    },
                    indent=1,
                )
                + "\n",
                encoding="utf-8",
            )
            print(f"\nwrote {out_root}/corpus-overview.csv "
                  f"and {len(results)} per-project profile(s) under {out_root}")
    else:
        if not args.quiet:
            print(render_terminal(results[0]))
        if not args.no_files:
            print(f"\nwrote {out_root}/cast-profile.{{md,json,html}} and "
                  f"{out_root}/tables/*.csv")

    worst = max((worst_level(d) for d in results), key=lambda l: LEVEL_ORDER.get(l, 0))
    failing = {"critical", "serious"} if args.strict else {"critical"}
    return 1 if worst in failing else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

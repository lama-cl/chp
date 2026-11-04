#!/usr/bin/env python3
"""Static contracts for C6g5v import/canonical history separation."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
state = (root / "include" / "CastHistory" / "AnalysisState.h").read_text()
visitor = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
index = (root / "include" / "CastHistory" / "SummaryIndex.h").read_text()
builder = (root / "scripts" / "build_summary_index.py").read_text()

for needle in (
    "directLocalObsSummary",
):
    assert needle in state, needle

for needle in (
    "directLocalEntityObservations",
    "isDirectLocalParameterObservation",
    "summarizeDirectLocalEntityObservations",
    '"directLocalSummary"',
):
    assert needle in visitor, needle

# Import remains explicitly bound to the legacy/import-authoritative field.
apply_begin = visitor.index("void applySolvedParamSummariesFromIndex")
apply_end = visitor.index("bool isTrivialReturnSummaryCore", apply_begin)
apply_body = visitor[apply_begin:apply_end]
assert "entry.solvedParamSummaries" in apply_body
assert "canonicalProjectParamHistories" not in apply_body

for needle in (
    "canonicalProjectParamHistories",
    "hasCanonicalProjectParamHistories",
):
    assert needle in index, needle

for needle in (
    "solve_recursive_scc_by_simple_paths_for_import",
    "solve_recursive_scc_by_simple_paths(",
    "canonical_param_summary_records_from_detail",
    "propagate_canonical_project_histories",
    "canonicalProjectParamHistories",
    "identity-edge-direct-local-base",
    "would revisit a parameter and replay its direct-local base",
    "display-only; excluded from solvedParamSummaries import channel",
):
    assert needle in builder, needle

print("C6g5v import/canonical source contract: OK")

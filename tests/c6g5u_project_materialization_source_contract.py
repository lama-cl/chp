#!/usr/bin/env python3
"""Static contracts for C6g5u direct-local C2 materialization."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
visitor = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
state = (root / "include" / "CastHistory" / "AnalysisState.h").read_text()
builder = (root / "scripts" / "build_summary_index.py").read_text()

for needle in (
    "directLocalObsSummary",
    "summarizeDirectLocalEntityObservations",
    "isInterproceduralObservationProvenance",
    "directLocalSummary",
):
    assert needle in visitor, needle
assert "directLocalObsSummary" in state

for needle in (
    "directLocalBaseIr",
    "directLocalBasePresent",
    "rawSummaryIr",
    "rawReportIr",
    "frozen-direct-local-base",
    "Append the immutable direct-local contribution",
    "directLocalSummary",
):
    assert needle in builder, needle

print("C6g5u project materialization source contract: OK")

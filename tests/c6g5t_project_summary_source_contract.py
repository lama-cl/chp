#!/usr/bin/env python3
"""Static contracts for C6g5t report materialization and C2 scheduling."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
visitor = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
index_h = (root / "include" / "CastHistory" / "SummaryIndex.h").read_text()
builder = (root / "scripts" / "build_summary_index.py").read_text()
runner = (root / "scripts" / "run_all_multitu_projects.sh").read_text()
runner_rp = (root / "scripts" / "run_all_multitu_projects-rp.sh").read_text()

for needle in (
    "Project-solved parameter histories (canonical):",
    "collectProjectSolvedParameterHistoryRecords",
    "canonical C2 materialization; direct AST-local",
    "fork=" ,
):
    assert needle in visitor, needle

for needle in (
    "ForkArmProvenanceEntry",
    "forkArmProvenance",
):
    assert needle in index_h, needle

for needle in (
    "canonical_guarded_ptrarith_transformer",
    "call-actual-fork",
    "forkArmProvenance",
    "buildMetadata",
    "inputSummaryFingerprint",
    "reuse-output-if-input-unchanged",
):
    assert needle in builder, needle

for script in (runner, runner_rp):
    for needle in (
        "project_index_has_importable_edges",
        "import rounds skipped: index has no imported cross-TU summary edges",
        "local-index-overlay",
        "--build-reason c1-raw",
        "--reuse-output-if-input-unchanged",
    ):
        assert needle in script, needle

print("C6g5t project-summary source contract: OK")

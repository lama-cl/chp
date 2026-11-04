#!/usr/bin/env python3
"""Static contracts for C6g5w1a diagnostics-only metadata completion."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISITOR = (ROOT / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
INDEX = (ROOT / "include" / "CastHistory" / "SummaryIndex.h").read_text()
BUILDER = (ROOT / "scripts" / "build_summary_index.py").read_text()

# p127 can arrive either as two OriginAlternative records or as one guarded
# Join. The extractor must split the latter only in diagnostics emission.
emit_start = VISITOR.index("  json::Array emitSummaryParamTransferEdgesJson")
emit_end = VISITOR.index("\n  json::Array emitSummaryDependencyEdgesJson", emit_start)
emit = VISITOR[emit_start:emit_end]
assert "expandGuardedJoinAlternative" in emit
assert "ObsKind::Join" in emit
assert "branch-family:guarded-origin-alternatives:" in emit
assert "branch-family:parallel-call-actuals:" in emit
assert "draft.alt = std::move(alt)" in emit

# SummaryIndex must support direct raw-edge records plus the C2 companion
# fallback, without mentioning branch metadata in a solver implementation.
assert 'root->getArray("summaryParamTransferEdges")' in INDEX
assert 'root->getObject("branchFamilyTransferMetadata")' in INDEX
assert "rehydratedFromProjectMetadata = true" in INDEX
assert 'transportChannel = "branchFamilyTransferMetadata"' in INDEX
assert "project-metadata-rehydrated=yes" in VISITOR
assert "rehydrationContract" in BUILDER

solver_start = BUILDER.index("def materialize_project_solved_param_summaries")
solver_end = BUILDER.index("\ndef build_solved_param_summary_debug", solver_start)
solver = BUILDER[solver_start:solver_end]
assert "branchFamily" not in solver
assert "branchArm" not in solver

print("C6g5w1a branch-family rehydration source contract passed")

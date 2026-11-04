#!/usr/bin/env python3
"""Source-contract guard for C6g5w1 transport, retained under C6g5w1d."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISITOR = (ROOT / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
INDEX = (ROOT / "include" / "CastHistory" / "SummaryIndex.h").read_text()
BUILDER = (ROOT / "scripts" / "build_summary_index.py").read_text()

start = VISITOR.index("  json::Array emitSummaryParamTransferEdgesJson")
end = VISITOR.index("\n  json::Array emitSummaryDependencyEdgesJson", start)
emit_body = VISITOR[start:end]
assert "branch-family:parallel-call-actuals:" in emit_body
assert "branch-family:guarded-origin-alternatives:" in emit_body
assert "diagnostics metadata" in emit_body
assert 'obj["branchFamily"]' in emit_body
assert 'obj["branchArm"]' in emit_body

assert "BranchFamilyTransferMetadataRecord" in INDEX
assert 'root->getArray("summaryParamTransferEdges")' in INDEX
assert "hasBranchFamilyTransferMetadata" in INDEX

assert "branch_family_transfer_metadata_summary" in BUILDER
assert '"branchFamilyTransferMetadata"' in BUILDER
# C6g5w1d preserves C6g5w1 transport but introduces a separate, narrow C2
# consumer.  The AST/C1 local solver remains metadata-free; consumption is
# restricted to the C2 tagged closure-key path.
assert "consumedByTaggedRecursiveSolver" in BUILDER
assert "def solve_tagged_recursive_scc_by_closure_key_paths_for_import" in BUILDER
local_start = VISITOR.index("  bool propagateOneNamedCallEdgeForSummaries")
local_end = VISITOR.index("\n\n\n  bool propagateOneCrossTUSCCNamedCallEdgeForSummaries", local_start)
local_body = VISITOR[local_start:local_end]
assert "recursiveClosureKey" not in local_body
assert "branchFamily" not in local_body

print("C6g5w1 branch-family transport source contract passed")

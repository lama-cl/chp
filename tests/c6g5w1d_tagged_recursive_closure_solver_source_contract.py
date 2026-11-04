#!/usr/bin/env python3
"""Static contracts for C6g5w1d's narrow metadata-consuming C2 solver."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = (ROOT / "scripts" / "build_summary_index.py").read_text()
VISITOR = (ROOT / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
INDEX = (ROOT / "include" / "CastHistory" / "SummaryIndex.h").read_text()
INSPECTOR = (ROOT / "scripts" / "inspect_summary_index.py").read_text()

assert "def solve_tagged_recursive_scc_by_closure_key_paths_for_import" in BUILDER
assert "recursive_closure_metadata_for_edge" in BUILDER
assert "active_closure_keys" in BUILDER
assert "recursive closure key already active on this arm-local path" in BUILDER
assert "seeded guarded local transformer closure key" in BUILDER
assert "excluded_roots=tagged_nodes" in BUILDER
assert "canonical_record_map_from_details" in BUILDER
assert "consumedByTaggedRecursiveSolver" in BUILDER
assert "tagged-recursive-closure-key-simple-scc-path" in BUILDER

# C6g5w1d is C2-only.  Live same-TU C++ composition remains unmodified.
start = VISITOR.index("  bool propagateOneNamedCallEdgeForSummaries")
end = VISITOR.index("\n\n\n  bool propagateOneCrossTUSCCNamedCallEdgeForSummaries", start)
local_solver = VISITOR[start:end]
assert "recursiveClosureKey" not in local_solver
assert "consumedByTaggedRecursiveSolver" not in local_solver

assert "bool consumedByTaggedRecursiveSolver = false" in INDEX
assert 'getBoolean("consumedByTaggedRecursiveSolver")' in INDEX
assert "recursive-closure-consumed-by=C2 tagged recursive" in VISITOR
assert "C1 live solver unchanged" in VISITOR
assert "recursive-closure-consumed-by=C2 tagged recursive summary materialization" in INSPECTOR

print("C6g5w1d tagged recursive closure solver source contract passed")

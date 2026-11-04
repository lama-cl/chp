#!/usr/bin/env python3
"""Static contracts for C6g5w1c key transport, retained under C6g5w1d."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISITOR = (ROOT / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
INDEX = (ROOT / "include" / "CastHistory" / "SummaryIndex.h").read_text()
BUILDER = (ROOT / "scripts" / "build_summary_index.py").read_text()
INSPECTOR = (ROOT / "scripts" / "inspect_summary_index.py").read_text()

start = VISITOR.index("  json::Array emitSummaryParamTransferEdgesJson")
end = VISITOR.index("\n  json::Array emitSummaryDependencyEdgesJson", start)
emit = VISITOR[start:end]

assert "guardedLocalPtrArithRecursiveClosureKey" in emit
assert "recursive-closure:guarded-local-ptrarith:formal=$" in emit
assert "d.recursiveClosureKey = familyId" in emit
assert "carrier->recursiveClosureKey" in emit
assert '"recursiveClosureKey", draft.recursiveClosureKey' in emit
assert "recursive-closure-key=" in VISITOR

assert "std::string recursiveClosureKey" in INDEX
assert 'getString(*family, "recursiveClosureKey")' in INDEX
assert 'getString(*fobj, "recursiveClosureKey")' in INDEX
assert "existing.recursiveClosureKey.empty()" in INDEX

assert '"recursiveClosureKey": str(' in BUILDER
assert "inconsistent recursiveClosureKey values" in BUILDER
assert "recursive-closure-key=" in INSPECTOR

# C6g5w1d consumes this key only in a separate C2 materializer; C1's live
# same-TU named-call summary propagation remains untouched.
solver_start = BUILDER.index("def materialize_project_solved_param_summaries")
solver_end = BUILDER.index("\ndef build_solved_param_summary_debug", solver_start)
solver = BUILDER[solver_start:solver_end]
assert "solve_tagged_recursive_scc_by_closure_key_paths_for_import" in solver
assert "recursiveClosureKey" in solver
local_solver_start = VISITOR.index("  bool propagateOneNamedCallEdgeForSummaries")
local_solver_end = VISITOR.index(
    "\n\n\n  bool propagateOneCrossTUSCCNamedCallEdgeForSummaries",
    local_solver_start)
local_solver = VISITOR[local_solver_start:local_solver_end]
assert "recursiveClosureKey" not in local_solver

print("C6g5w1c recursive closure-key transport contract remains intact")

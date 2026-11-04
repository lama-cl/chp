#!/usr/bin/env python3
"""Static contracts for C6g5w1b's metadata-only p127 extraction."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISITOR = (ROOT / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
INDEX = (ROOT / "include" / "CastHistory" / "SummaryIndex.h").read_text()
BUILDER = (ROOT / "scripts" / "build_summary_index.py").read_text()

start = VISITOR.index("  json::Array emitSummaryParamTransferEdgesJson")
end = VISITOR.index("\n  json::Array emitSummaryDependencyEdgesJson", start)
emit = VISITOR[start:end]
assert "guardedLocalPtrArithArmsForActual" in emit
assert "guarded-local-ptrarith" in emit
assert "branchArms" in emit
assert "The edge itself is not\n      // duplicated" in emit
assert "pointerOperandOfPointerArithmetic" in emit
assert "if (draft.branchArms.empty())" in emit
assert '"guardText", arm.guardText' in emit

assert "std::optional<std::string> armGuard" in INDEX
assert 'tobj->getArray("branchArms")' in INDEX
assert 'getString(*arm, "guardText")' in INDEX
assert "arm-guard=" in VISITOR
assert "branchArms" in BUILDER
assert '"guardText": str(arm.get("guardText") or "")' in BUILDER

solver_start = BUILDER.index("def materialize_project_solved_param_summaries")
solver_end = BUILDER.index("\ndef build_solved_param_summary_debug", solver_start)
solver = BUILDER[solver_start:solver_end]
assert "branchFamily" not in solver
assert "branchArms" not in solver

print("C6g5w1b guarded-local ptrarith source contract passed")

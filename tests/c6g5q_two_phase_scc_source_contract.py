#!/usr/bin/env python3
"""Static contract checks for C6g5q's two-phase named-call SCC refactor."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
source = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()


def region(start: str, end: str) -> str:
    a = source.find(start)
    b = source.find(end, a + len(start))
    if a < 0 or b < 0:
        raise AssertionError(f"missing source region: {start!r} .. {end!r}")
    return source[a:b]

cast_driver = region("void computeNamedCallSummaryFixpoint()", "void computeCrossTUSCCParamSummaryFixpoint()")
pointer_phase = region("bool computeNamedCallPointerUseSCCFixpoint(", "void computeNamedCallSummaryFixpoint()")

assert "const bool edgeChanged = summaryChanged;" in cast_driver
assert "propagateOneNamedCallEdgeForPointerUses(" not in cast_driver.split(
    "bool computeNamedCallPointerUseSCCFixpoint(", 1)[0]
assert "computeNamedCallPointerUseSCCFixpoint(" in cast_driver
assert "if (!budgetExhausted)" in cast_driver
assert "namedCallFixpointActiveDomain = \"pointer-use\";" in pointer_phase
assert "propagateOneNamedCallEdgeForPointerUses(" in pointer_phase
assert "propagateOneNamedCallEdgeForSummaries(" not in pointer_phase
assert "recursiveSCCCastCandidateAddsSemanticFact" in source
assert "pointer-use changes never reopen cast-history SCC solving" in source
assert "++namedCallPointerUseSCCReopenedCastSCC" not in source

for fixture in ("p126_recursive_parallel_observation_dedup",
                "p127_guarded_recursive_ptrarith_membership_projection"):
    project = root / "test_projects" / "projects" / fixture
    assert (project / "a.c").is_file(), fixture
    assert (project / "compile_commands.json").is_file(), fixture
    assert (project / "expected" / "README.expected.txt").is_file(), fixture

print("C6g5q two-phase SCC source contract: OK")

#!/usr/bin/env python3
"""Static contract checks for C6g5r's phase-2 operation-set solver."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()


def region(start: str, end: str) -> str:
    a = source.find(start)
    b = source.find(end, a + len(start))
    if a < 0 or b < 0:
        raise AssertionError(f"missing source region: {start!r} .. {end!r}")
    return source[a:b]

pointer_transfer = region(
    "bool propagateOneNamedCallEdgeForPointerUses(",
    "// C6c: materialize same-TU field-function-pointer pointer operations",
)
pointer_phase = region(
    "bool computeNamedCallPointerUseSCCFixpoint(",
    "void computeNamedCallSummaryFixpoint()",
)

assert "renderNamedCallPointerUseSCCOperationKey" in source
assert "addUniqueNamedCallPointerUseSCCOperationRecord" in source
assert "frozenCastAttachmentForPointerUseSCC" in source
assert "pointerUseSCCOperationProjected=*/true" in pointer_transfer
assert "renderNamedCallPointerUseSCCOperationKey(transferred)" in pointer_transfer
assert "if (frozenDestinationAttachment)" in pointer_transfer
assert "composeObsPath(prefix, calleeUse.attachPath)" in pointer_transfer
assert "recursiveUseExtendsCallerDirectUse(" not in pointer_transfer
assert "callerDirectUseIt" not in pointer_transfer
assert "pointer-use-scc-operation-insertions" in source
assert "pointer-use-scc-frozen-attachment-overlays" in source
assert "propagateOneNamedCallEdgeForSummaries(" not in pointer_phase
assert "++namedCallPointerUseSCCReopenedCastSCC" not in source

for fixture in (
    "p124_mutual_three_recursion_no_if_widening",
    "p126_recursive_parallel_observation_dedup",
    "p127_guarded_recursive_ptrarith_membership_projection",
    "p128_recursive_mutual_pointer_use_operation_identity",
):
    project = root / "test_projects" / "projects" / fixture
    assert (project / "a.c").is_file(), fixture
    assert (project / "compile_commands.json").is_file(), fixture

print("C6g5r pointer-use SCC source contract: OK")

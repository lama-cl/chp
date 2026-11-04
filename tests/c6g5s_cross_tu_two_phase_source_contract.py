#!/usr/bin/env python3
"""Static contract checks for C6g5s's project/import two-phase SCC solver."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
visitor = (root / "include" / "CastHistory" / "CastHistoryVisitor.h").read_text()
index_header = (root / "include" / "CastHistory" / "SummaryIndex.h").read_text()
index_builder = (root / "scripts" / "build_summary_index.py").read_text()


def region(text: str, start: str, end: str) -> str:
    a = text.find(start)
    b = text.find(end, a + len(start))
    if a < 0 or b < 0:
        raise AssertionError(f"missing source region: {start!r} .. {end!r}")
    return text[a:b]

project_solver = region(
    visitor,
    "void computeCrossTUSCCParamSummaryFixpoint()",
    "static std::string eraseWhitespace",
)
project_pointer = region(
    visitor,
    "bool propagateCalleePointerUsesAlongNamedCallEdgeForProjectSCCPhase2(",
    "bool propagateOneCrossTUSCCNamedCallEdgeForPointerUses(",
)

for needle in (
    "crossTUSCCComponentRequiresProjectTwoPhaseSolve",
    "crossTUSCCComponentIsKnownLocalOnly",
    "skipped-local-only-sccs",
    "resetProjectSCCCastFactMembership",
    "projectSCCSemanticFactMembership=*/true",
    "projectSCCOperationMembership=*/true",
    "crossTUSCCParamFixpointActiveDomain = \"cast-history\"",
    "crossTUSCCParamFixpointActiveDomain = \"pointer-use\"",
    "project-pointer-use-scc-worklist-quiescent",
):
    assert needle in project_solver, needle

assert "addObservationToFunctionParamForProjectSCC" in visitor
assert "renderAllFunctionParamObsKeys" not in project_solver
assert "renderAllFunctionParamUseKeys" not in project_solver
assert "propagateOneNamedCallEdgeForPointerUses(" not in project_solver
assert "frozenCastAttachmentForPointerUseSCC" in project_pointer
assert "pointerUseSCCOperationProjected=*/true" in project_pointer
assert "renderNamedCallPointerUseSCCOperationKey(transferred)" in project_pointer
assert "crossTUSCCPointerUseReportOnlyEvidenceMerges" in project_pointer

for needle in (
    "concreteEdgeCount",
    "localEdgeCount",
    "importedEdgeCount",
    "translationUnits",
    "hasEdgeTopology",
    "isLocalOnly",
):
    assert needle in index_header, needle

for needle in (
    "edgeTopologyVersion",
    "crossTUCandidate",
    "importedEdgeCount",
    "localEdgeCount",
):
    assert needle in index_builder, needle

for fixture in (
    "p126_recursive_parallel_observation_dedup",
    "p129_cross_tu_mutual_three_two_phase",
    "p130_cross_tu_recursive_parallel_observation_dedup",
):
    project = root / "test_projects" / "projects" / fixture
    assert (project / "compile_commands.json").is_file(), fixture

print("C6g5s cross-TU two-phase source contract: OK")

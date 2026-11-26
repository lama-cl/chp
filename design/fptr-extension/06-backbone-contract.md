# 06 — Backbone contract (for implementers and reviewers)

Decision **D17** (Proposed): the fptr extension is built *beside* the existing `CastHistoryVisitor` and connected to it only at the **seams** listed below. This page is the contract every milestone PR is checked against.

## 1. Invariants (must hold after every milestone)

0. **Round 4:** no semantic edit to existing visitor code. All fptr flow logic lives in the CFG solver ([08](08-cfg-fptr-resolution.md)); the visitor only consults its table (S6) and records edges (S9).

1. **Traversal is unchanged.** Same nodes, same order, each node visited once. Every `Traverse*` override still delegates to `RecursiveASTVisitor<CastHistoryVisitor>::Traverse*` exactly as today.
2. **Non-fptr state is untouched.** No change to the save/restore or merge of `valueFlows`, `originState`, `fieldOriginState`, `fieldValueFlows`, `aliasParentState`, `pointerStorageAliases`, `structPointerObjectAliases*` or `localValueSelfPath*` (`TraverseFunctionDecl :1850`, `TraverseIfStmt :3074`).
3. **The transfer engine is untouched.** `VisitCallExpr` (`:5703`) and the C3c/C5/C6 composition paths (`applyImportedFunctionPointerTargetSummary :5198`, …) receive target sets; they do not learn how those sets were found.
4. **The categories are untouched** (D1). `paperPrimaryCategory` (`:21200`) is not edited.
5. **Text report.** The text report is byte-identical for every non-fptr example. For fptr fixtures it changes only in the sections an ADR names.
6. **Existing containers stay.** `std::unordered_map`/`std::set` L0 maps stay as they are. New code uses LLVM containers (CLAUDE.md) and converts at the seam.

## 2. Seams

Each seam is a small, named edit, at most about 15 lines unless the PR justifies more. A PR lists the seams it touches. Any other edit to `CastHistoryVisitor.h` or `src/main.cpp` needs a new ADR.

| Seam | Anchor (grep) | Allowed change | Reviewer check | Milestone |
|---|---|---|---|---|
| **S1** pre-pass | `src/main.cpp`: `visitor.TraverseDecl(context.getTranslationUnitDecl())` | before that line: run `FptrCFGSolver` over the TU and hand the result table to the visitor (round 4: replaces `FptrSlotCollector`) | the solver records no cast observations and never mutates visitor state | M2a |
| **S6** lookups | `resolveFunctionValueSet(`, `resolveFunctionValue(`, `functionPointerSelectionGuardForTarget(` | at the top of each: if the CFG table has an entry for this `Expr*`, return it (targets, or the guard evidence for a target); otherwise fall through to the existing AST code | fallthrough path byte-identical; F01–F41 controls unchanged | M2a |
| **S8** report | `json::Object paperSubjectJson(` | add the `indirectCallResolution` key from `fptr.facetFor(entity)` | text report unchanged in M1 | M1 |

| **S9** edges | next to `recordSameTUFieldFunctionPointerNamedCallEdges(` | additive recorder: a resolved local-variable or L1 fptr call appends a `NamedCallEdge` (with `layer`), so the existing Tarjan and two-phase solves see it (F27) | SCC report shows the recursive component; no change to SCC/fixpoint code | M4 |

Seams S2–S5 and S7 (AST control-flow patches) were retired in round 4 (D24).

The visitor also gains **one member**, `FptrResolution fptr;`, with its accessor. That does not count as a seam.

## 3. Where new code lives

| File | Contents |
|---|---|
| `include/CastHistory/FptrResolution.h` | `SlotKey` (D7′ outermost-path keys), `TargetSet` (`⊥ / {..} ≤ K / ⊤(reason)`, stable order, D21), `L0Fact` tri-state (D18), `FptrSlotCollector`, `FptrFlowTracker`, `resolveSlot`, facet builder |
| `include/CastHistory/LibraryCallbackModels.def` | LIB table (M7) |
| `scripts/build_summary_index.py` | slot merge/solve (M8); additive keys only |
| `schemas/*.json` | additive; `schemaVersion` bump recorded in REVIEW.md |

## 4. Diff budget per milestone (guideline)

| Area | Budget | Over budget → |
|---|---|---|
| `CastHistoryVisitor.h` + `main.cpp`, at the listed seams | ≤ 60 changed lines per milestone (M2: ≤ 120) | split the milestone, or write an ADR explaining why |
| `FptrResolution.h` | no limit, but each new class gets a header comment naming the facts it serves | — |
| fixtures, markers, evidence table | as needed; `@expected` edits only in their own commit (04 rules) | — |

## 5. Task card template (copy into the PR description)

```markdown
### <Milestone id>: <descriptive name>            (PR title = this line, per CLAUDE.md)
- Claims: F.., X..                    (from 04-implementation-plan.md)
- Owner gate: D.. accepted on <date>  (link to REVIEW.md entry) / "none"
- Seams touched: S.., S..             (06-backbone-contract.md)
- Visitor diff: <n> lines (budget <m>)
- Commands run (all green):
  - [ ] ctest -L fptr                         (before)
  - [ ] check_fptr_evidence.py --check expected --only <claims>
  - [ ] check_fptr_evidence.py --pin-baselines --only <claims>
  - [ ] ctest -L fptr                         (after; no unclaimed drift)
  - [ ] run_all_examples_normalized.sh + run_all_multitu_projects.sh
  - [ ] ctest -LE known-failing
- Unclaimed fixtures that changed: none / <id + reason + REVIEW.md link>
- Deviations from 02/06: none / <ADR id>
```

## 6. Stop conditions (agents)

Stop and report to the owner instead of continuing when:

- an unclaimed fixture drifts, and the cause is not an obvious bug in the change;
- the owner gate of the milestone is not met;
- a change is needed outside the seams;
- `llvm-config --version` differs from the version recorded in `examples/fptr-meta.info` (after the re-pin).

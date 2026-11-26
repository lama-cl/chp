# fptr extension — review log

A summary for human review of the design discussion: what was asked, what was found, what the owner decided, and where the author pushes back or has concerns. Detailed material is in the numbered documents; this page is meant to be read in a few minutes.

## 1. Request (2026-11-18)

Extend Cast History, which classifies `void*` into 7 categories using casts, aliases and operation profiles, so that it covers **function pointers**. Deliverables:

- **(0)** Explain what makes fptrs hard.
- **(1)** Minimal single-TU and cross-TU C examples, one fact each.
- **(2)** A design for fptr data/control-flow tracking.
- **(3)** This review log.

Follow-up directives:
- Evidence must exist in `examples`/`test_projects` before any code change.
- Tests must be self-contained and incremental.
- Find fptr uses not yet discussed, with real-world citations.

## 2. What we learned (evidence, not opinion)

The tool was built (LLVM 18.1.3) and probed with about 30 throw-away programs, then with the 48 committed fixtures. The full table is in [evidence/evidence-table.md](evidence/evidence-table.md).

- **fptr support already exists and is substantial.** It covers local flow-sensitive targets, `if` merges, per-base field targets, `$k.$i` higher-order summaries, and cross-TU known/multi-target import (C3c/C5/C6, projects p05, p12–p17, p34–p48, p85–p91). The gap is **target discovery**, not composition.
- **Coverage gaps:** dispatch tables, `?:`, globals set elsewhere, factories/out-params, vtables and OOP `void *self`, FSMs, callback registries, executors, library callbacks (`pthread_create`, `qsort`), `va_arg`, equality checks, and return-through-fptr. In each case the `void*` falls back to `OpaqueTransportOrViewOnly` because the callee's evidence never reaches it.
- **Three soundness bugs** (wrong answers, not just missing ones):
  - `switch` arms keep only the last target (F09 → `double*` only; X11 too).
  - Loop back-edge reassignment is ignored (F10 → `int*` only; X12 too).
  - *Found during this work:* a global's initializer is treated as its current value in every function, even when another function reassigns it (F35 → `int*` only). Related order dependence: the same global defined after its use is unresolved (F34).
- **Mislabelled mediation.** `callbackMediation` reports `Resolved*` for F07, F13, F20, F24, F27 and F28, although no type evidence was transferred.
- **Already correct** (now pinned as controls): F01, F02, F03 (by traversal order only), F25, F31. F14 is correct on types but does not flag the incompatible-signature call.

## 3. Owner decisions

| # | Question | Owner's answer |
|---|---|---|
| D1 | How to report fptr-dependent subjects in the 7-category scheme | Keep the 7 categories; **add a resolution facet**. *"OpaqueTransportOrViewOnly is just the category of a pointer in the scope of its visible history. Adding a resolution facet makes more sense that can be combined later with different categories."* |
| D2 | Resolution strategy when flow-sensitive tracking fails | **Layered L0 → L1 → L2 with precision tags** |
| D4 | switch / loop soundness bugs | **Fix inside the fptr extension, fptr state only**; document both cases in `examples` / `test_projects` (done: F09, F10, X11, X12) |
| D5/D6 | Library-invoked callbacks and `dlopen`/`dlsym` | **Library model + dlsym unknown** |
| D10 | Process | Evidence in fixtures before code; self-contained, incremental tests; docs readable by humans and actionable by agents |

## 4. Author's design in one paragraph

1. A pre-pass collects **slot facts**: which functions may be stored in each global, array element, record field (by type), return value and out-parameter.
2. At each indirect call, one resolver tries, in order: local flow state, guard refinement, slot facts, parametric `$k`, the library model, and optionally a signature fallback. Each target carries its layer.
3. Resolved targets feed the **existing** transfer engine: forward into parameters, backward via returns and return aliases. Callback/context pairs are turned into ordinary higher-order summaries at registration sites, so they never cross.
4. Cross-TU, the same facts are serialized into `fptrFacts`, merged and solved by `build_summary_index.py`, and consumed in import rounds.
5. A per-subject `indirectCallResolution` facet (`exact | set | parametric | signature | unresolved | external-*` plus the layer) makes coverage measurable without touching the 7 categories.

Details: [02-design.md](02-design.md). Milestones: [04-implementation-plan.md](04-implementation-plan.md).

## 5. Pushback and concerns

| # | Concern | Position / mitigation |
|---|---|---|
| C1 | **Guard policy changes category counts** (D8). Whether a multi-target set is "guarded" decides between `GuardedActiveReinterpretation` and `UnguardedMultiTarget…`. | Proposed rule: a selector visible at the call → guarded; temporal selection (loops, globals mutated elsewhere) → unguarded. **Needs owner sign-off before M2.** |
| C2 | **Object-insensitive record slots may over-merge** (D7) in code bases that reuse one ops type for unrelated purposes. | Accepted trade-off for C vtables; tagged L1; K cap; per-object L0 overlay. Measure on bench-3c (icecast, libarchive) at M3. |
| C3 | **`callbackMediation` already says `Resolved*` without a transfer.** Fixing the label changes existing outputs and tables. | Proposed: do not change the label; add `composed` to the facet plus a validator warning. Owner to confirm. |
| C4 | **The same syntactic switch/loop traversal is used for origin, field and alias state**, so similar unsoundness likely exists for data pointers. D4 limits the fix to fptr state. | Recommend a separate issue; not addressed here. |
| C5 | **CLAUDE.md asks for LLVM 19+ and `FileCheck`/lit tests in `test/`.** The environment had LLVM 18.1.3, and the repo has `tests/` (Python smoke tests) with no lit config. | Built and pinned on 18.1.3. Evidence is checked by a Python marker checker for now; migrating to FileCheck is milestone M9. Re-pin if LLVM 19 changes the output. |
| C6 | **Fixture count (48) vs "minimal".** | Each fixture is minimal and shows one fact. The count reflects the number of distinct facts; the milestones claim them in small groups. |
| C7 | **System headers** (`pthread.h`, `dlfcn.h`) in F21, F23, X09, X10 make those fixtures depend on a POSIX sysroot. | Acceptable on Linux CI; on other hosts, declare prototypes locally. |
| C8 | **dlsym name hints** (X10) could tempt composing a project function that is not what gets loaded. | D6: hints are reported, never composed. |
| C9 | **L2 signature fallback** is noisy in large code bases (every `void (*)(void*)`). | Off by default, capped, always tagged; evaluated last (M9). |
| C10 | Pre-existing stray `p16_direct_cross_tu_recursion/expected/README.expected.txt.{rej,orig}` files. | Untouched; flagged for cleanup. |

## 6. Open questions for the owner

1. **D8 guard policy** (C1): accept the rule "selector visible at the call → guarded, temporal → unguarded"?
2. **D7 record slots**: accept object-insensitive (record type + field) slots for L1?
3. **D12 / C3**: keep the `callbackMediation` label as is and add `composed`, or fix the label?
4. **D6 hint**: report `dlsym` name matches as hints, or suppress them entirely?
5. **D11 caps**: is K = 16 / L2 ≤ 8 with L2 off by default acceptable?
6. **C4**: open a separate issue for switch/loop handling of non-fptr state?

## 6b. Round 2: plan review (2026-11-25)

**Asked:** is the incremental, gated plan solid? Does it keep the visitor backbone? Record the discussion, diagrams and decisions so that humans can review them (with a short guide) and agents can act on them.

**Answer:** the direction is solid, but the plan was not yet safe to hand to agents. Full reasoning: [05-plan-review.md](05-plan-review.md). Backbone contract: [06-backbone-contract.md](06-backbone-contract.md). Guide: [REVIEWING.md](REVIEWING.md).

What changed:
- **Gate fixed.** The owner's untracked probes had broken `check_fptr_evidence.py`. They now live byte-identical in `examples/probes/`. Discovery only accepts `fptr-NN-*.c` and names stray files. ctest test `fptr-baseline` (label `fptr`) runs the gate: 56 fixtures, 65 subjects, drift 0.
- **Two more wrong answers found and pinned.** F36: a one-armed `if` after a multi-target merge drops targets, because `TraverseIfStmt` never snapshots `functionPointerTargetSets`. F43 `run`: a global overwritten by an external call elsewhere still reports the initializer target.
- **Three design corrections.** D7′ (record keys use the outermost visible path; F37, F42), D18 (L0 tri-state; F43), D21 (deterministic order).
- **Backbone.** New code goes beside the visitor; the visitor changes only at eight seams. One seam (S5) changes existing semantics, and only to fix F36.
- **Plan shape.** M0.5 inserted; M3 → M3a/b/c; M8 → M8a–d; every milestone names its owner gate.

**Owner answers this round**

| Question | Answer |
|---|---|
| What to do with the 8 probes | Move the originals to `examples/probes`; add corrected, marker-carrying copies as F36+ only if they add something new. *Done: F37–F41 come from the probes; F36, F42, F43 come from the review.* |
| What should the struct probes pass to `s.fn()`? | "Depends on the example: could be the sibling member or self. Is a separate void* ctx well supported?" *Yes. A separate ctx is the most common idiom. All three readings are fixtured (probes README).* |
| LLVM version | "LLVM 23 is available, show me how you checked." *Shown in 05 §6: the alternatives link goes to `/opt/llvm-23`, which the agent's shell cannot see. Re-pinning is an owner step (REVIEWING.md).* |

**New concerns**

| # | Concern | Position |
|---|---|---|
| C11 | The LLVM re-pin may move baselines. **Update 2026-11-25:** LLVM 23.1.4 is reachable from the agent's shell, but only as `bin/`; there are no libs, headers or cmake configs, so the tool cannot be built against it there (configure falls back to llvm-18/clang-14 and the link fails). The llvm-18 build was restored. | Re-pin in its own commit before M1, on a host with the LLVM 23 development install; record the version in `examples/fptr-meta.info`. Steps in REVIEWING.md. **Superseded (later 2026-11-25):** `/opt/llvm-23` does include `lib/cmake`, headers and static libraries. The tool was built against it once the clang 18 `C_INCLUDE_PATH`/`CPLUS_INCLUDE_PATH` were unset and the CMake cache was fresh. Result: fptr drift 0 (no re-pin needed); version recorded in `examples/fptr-meta.info`. |
| C12 | S5 edits an existing backbone function. | Justified by the F36 defect. The change is limited to fptr maps. F01/F02/F38–F41 guard it. |
| C13 | D7′ cast rule (F42) can widen sets in GObject-style code. | K cap, `L1` tag; measure on bench-3c at M3c. |
| C14 | Switch arms without a CFG (S3) can be fragile. | Fall back to `entry ∪ stores`, unguarded. If it fires often on bench code, use a CFG for switches only (option c in 05 §3). |
| C15 | Controls F38–F41 show `xfail` until M1, which may look like failures. | They are claimed by M1. Category and types are already guarded by `@baseline`. |
| C16 | Marker checks do not show text-report drift. | At M1, add the fptr fixtures to the `regress.py` set (D19). |
| C17 | `ctest -LE known-failing` on the llvm-18 build fails only in `regress-projects`. That failure predates this change, which touches no C++. The diffs are the clang resource-dir in the recorded command line and **identical lines printed in a different order** (expected files from 20.1.2). | The reordering is live evidence for D21 (deterministic order). **Re-checked on LLVM 23.1.4:** same picture. The text reports have identical content once sorted, and the JSON is identical. Output is stable across 3 runs of one binary, so the order depends on the build, not on the run. Fix by regenerating with `regress.py update` on LLVM 23 (owner decision, regression-testing.md D1), or by D21. Everything else is green on 23. |

**Owner decisions after round 2**

| # | Decision |
|---|---|
| D12 | **Accepted with amendments:** order `… < external-modeled < set < parametric = exact`; `composed` renamed `indirectHistoryComposition`. The author's pushback (open vs closed parametric) is recorded in D12 and adds `parametricClosure`. The facet is emitted in paper facts for every subject (`none` when no indirect call); the text report is unchanged. M1 is unblocked. |
| Process | M1 uses mixed models: Opus for C++ seam work, a Sonnet subagent for schema/validator/aggregator, direct shell for test runs. |

**New open questions**

7. **D17**: accept the seam contract and the diff budgets as binding for agents?
8. **D7′**: accept outermost-path record keys, plus the cast-union rule (or `unresolved` when it cannot be computed)?
9. **D18**: accept the L0 tri-state?
10. **D20**: accept the milestone splits and the "stop if the owner gate is open" rule?
11. **F42 expectation**: a sound union (`set@L1`, as pinned) or `unresolved`?

## 6c. M1 landed (2026-11-25): resolution facet, reporting only

- **What.** Every paper-facts subject now carries `indirectCallResolution` (schema **v9 → v10**; the "3 → 4" written at M0 was stale). The text report is unchanged.
- **Code.** New `include/CastHistory/FptrResolution.h` (facet builder: a pure function over existing pointer-use records). The visitor changes only at **seam S8**: +34/−1 lines in `paperSubjectJson`, the closure helper and the version line. Python side: schema, `validate_paper_facts.py` (rules + the `Resolved*`/`none` warning), `aggregate_paper_facts.py` (category × resolution CSV), `aggregate_declared_pointer_populations.py` (accepts v10), smoke test `tests/fptr_resolution_facet_smoke.py`.
- **Claims met:** F01, F02, F25, F31, F38, F39, F40, F41 (13 subjects).
- **Deviations.**
  - **F14 moved to M2.** The incompatible fn-type cast is not recorded anywhere today, so `sigmismatch=true` needs the cast-normalisation step of the resolver (seam S6). It was not claimable through S8.
  - **F43 `run_local::ctx` flipped unclaimed.** It was already correct; the facet completed it (`unresolved@-`). Moved into M1's claim.
- **What the facet shows today** (honest, not fixed): `callbackMediation=Resolved*` with `indirectHistoryComposition=none` for F07, F13 `caller`, and F20 `submit`, which the validator flags with a warning. F09 and F36 report `exact` for a single target; that is their known unsound answer, fixed in M2. F31 `apply` is `parametric/open` (no callers) and F39 `hof` is `parametric/closed`.
- **M1 approximation (documented in `FptrResolution.h`).** A site is `parametric` when the subject's own summary still contains a `$k.$i` reference. The callee text is `<indirect>`, or `f.$k -> <indirect>` when the call is reached through a direct call. M2's resolver records the real callee expression.
- **Verification.** Claimed fixtures meet `@expected`. Baseline drift is 0 across all 56 fixtures (65 subjects), with no `@expected` edited. ctest passes except the pre-existing `regress-projects` (C17). 129/129 examples and 145/145 projects pass.
- **Models and tokens.** Schema, validator and aggregator were done by a Sonnet subagent (about 79k tokens, 16 tool calls, about 2 min). The C++ facet builder, seam S8, verification and docs were done in the main Opus session. The main session cannot measure its own token use.

## 6d. Round 3: the existing engine, checked against `main` (2026-11-25)

**Asked:** the docs say the CFG is approximated from the AST. Compare against `main`, look at the SCCs, fixpoint convergence and whether a CFG exists, and revise the docs. Reference: [07-existing-engine.md](07-existing-engine.md).

**Findings** (`main` = `codex/main` = this branch's base):
- **No CFG.** Only `if/else` merges state. Switch, loops, `?:` and `&&`/`||` are traversed once. Early exits are not modelled (new fixture F44).
- **Inter-procedural machinery is mature:**
  - two Tarjans: same-TU in C++, project in `build_summary_index.py`;
  - capped two-phase fixpoints (16);
  - widening tokens;
  - import rounds (default **1**).
- **Termination rests on caps.** The same-TU termination status is in the text report only, not in JSON (G6).

**Corrections to earlier docs:**
- F27's root cause: there is no edge because the table call is unresolved, not because SCCs exclude indirect edges.
- Field and imported fptr edges *are* SCC edges; local-variable fptr calls are not (B7).
- The rounds default is 1, not 2.
- `build_summary_index.py` runs no solver.

**Agents:** two Sonnet Explore agents, about 53k and 87k tokens, plus direct probes in the main session.

## 6e. Round 4: CFG for fptr resolution (2026-11-25)

**Owner's request:** discuss a CFG addition supporting **loops > conditions > goto** and their combinations, used for fptr resolution, with recursion and cross-TU composition. Design: [08-cfg-fptr-resolution.md](08-cfg-fptr-resolution.md).

**Decisions**

| # | Decision | Status |
|---|---|---|
| D24 | `clang::CFG` solver as a TU pre-pass. The visitor only looks results up (S6) and records edges (S9). No semantic edit to existing visitor code. Supersedes S2–S5 and S7, D15 and D23 | Proposed |
| D25–D28 | edge-label guards as evidence; TU summaries with an on-the-fly call graph and capped SCC fixpoint; cross-TU fptr summaries solved in C++ import rounds (resolves D22); no-CFG fallback plus cross-check | Proposed |
| D29 | fn-type casts get a cast history. `fnptr-transport`: no flag. `fnptr-param-variance`: flagged `ubKind=param-type-incompatible` with the reason (UB, kept for later UB and multi-platform analyses). `fnptr-incompatible`: flagged `ubKind=signature-incompatible` | **Accepted (owner)** |
| D30 | dispatch correlation (`single-view` / `object-correlated` / `per-iteration-subject` / `unrelated`) replaces the D8 guard policy | **Accepted (owner; "unrelated" is the owner's term)** |

**Discussion**
- **Owner.** Loops "while modeled well are still not well-defined for history". An unguarded tag is not interesting for fptrs. Table dispatch is safe when reads and writes happen on the same object through the same function. Use the subject and the function set to decide mismatch.
- **Author.** Agreed. A loop contributes only (1) sound target sets and (2) whether the same subject meets different targets across iterations.
- **Author's pushback.** Guards remain the evidence for object correlation, for example `switch (o->tag)`.

**Evidence** (fixture before code)
- F45–F56 and X14 are pinned. Four more subjects are **unsound today**: F46 `run_while`, F47, F50 and F51.
- F14's expected result gains `ubkind=param-type-incompatible`, in its own commit.
- The checker accepts the `ubkind` and `dispatch` keys.

**Plan re-cut:** M2a loops, M2b conditions, M2c goto/combinations, M3b gains F54/F55, M4 gains F52, M8c gains X14.

**New open questions**

12. Accept D24–D28 (the CFG solver architecture)?
13. Runtime budget for the CFG pre-pass: is ≤ 10 % on bench acceptable?
14. Should `per-iteration-subject` dispatch be reported per call (the current proposal) or as its own category flag?

## 7. Change log

| Date | Change |
|---|---|
| 2026-11-18 | Probes; challenges identified; owner decisions D1, D2, D4, D5/D6, D10 |
| 2026-11-20 | M0 landed: 35 single-TU fixtures (`examples/fptr-01..35`), 13 cross-TU projects (`p132`–`p144`), `scripts/check_fptr_evidence.py`, design docs. F34/F35 (global-initializer unsoundness) found while writing the fixtures. |
| 2026-11-25 | Review round 2 (05, 06, REVIEWING.md). Probes moved to `examples/probes/`. F36–F43 added and pinned (56 fixtures, 65 subjects, drift 0). Checker discovery made strict. ctest label `fptr`. D7′, D17–D21. M0.5 inserted; M3/M8 split. LLVM 23 re-pin pending (owner step). |
| 2026-11-25 | Built on LLVM 23.1.4: fptr drift 0 (no re-pin), 129/129 examples and 145/145 projects pass. Portable build fixes: RTTI follows LLVM, default `-resource-dir`, `getBeginLoc()` for break/continue. The shell profile's clang 18 `C_INCLUDE_PATH`/`CPLUS_INCLUDE_PATH` must be unset. |
| 2026-11-25 | M1 landed: `indirectCallResolution` facet (paper facts v10), seam S8 only. 8 fixtures (13 subjects) met, plus F43 `run_local`. F14 moved to M2. |
| 2026-11-25 | Round 3: 07-existing-engine.md (no CFG; SCC/fixpoint model), F44, D22/D23, corrections. |
| 2026-11-25 | Round 4: 08-cfg-fptr-resolution.md; D24–D30 (D29/D30 accepted); F45–F56, X14 pinned; F14 gains ubkind; M2 re-cut into M2a/b/c. 70 fixtures, 80 subjects, drift 0. |

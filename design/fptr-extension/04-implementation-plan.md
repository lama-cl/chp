# 04 — Incremental implementation plan

Each milestone **claims** a list of fixtures. A milestone is done when exactly its claimed fixtures switch from `xfail` to `expected-met`, and every other fixture keeps its pinned baseline.

## Agent workflow (every milestone)

```bash
# 0. build on LLVM 23 (the pinned version); clean-shell recipe in REVIEWING.md §LLVM
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=$(llvm-config --cmakedir) \
      -DClang_DIR=$(llvm-config --libdir)/cmake/clang -G Ninja && ninja -C build
# 1. before touching code: the evidence must be green
(cd build && ctest -L fptr --output-on-failure)   # = check_fptr_evidence.py --check baseline
# 2. implement the milestone (touch points in 02-design.md §10)
# 3. acceptance: claimed fixtures meet @expected
python3 scripts/check_fptr_evidence.py --check expected --only <claimed ids>
# 4. re-pin ONLY the claimed fixtures (also regenerates project READMEs)
python3 scripts/check_fptr_evidence.py --pin-baselines --only <claimed ids>
# 5. nothing else drifted
python3 scripts/check_fptr_evidence.py --check baseline
# 6. full regression per CLAUDE.md: all examples and all test_projects
(cd build && ../scripts/run_all_examples_normalized.sh)
python3 scripts/regenerate_compile_commands.py
(cd build && ../scripts/run_all_multitu_projects.sh)
# 7. refresh the evidence table, then log decisions/deviations in REVIEW.md
python3 scripts/check_fptr_evidence.py --markdown design/fptr-extension/evidence/evidence-table.md
```

**Rules**
- If an *unclaimed* fixture changes in step 5, stop. Either the change is a regression (fix it), or it is a deliberate improvement. In that case, move the fixture into this milestone's claim list and record the reason in REVIEW.md.
- Never edit `@expected` to match the output. If an expectation turns out to be wrong, change it in its own commit with the reason recorded in REVIEW.md.
- A milestone that changes output formatting must also update or add `FileCheck` tests (CLAUDE.md); see M9.

## Milestones

Revised in review round 2 (2026-11-25; D17, D20): M0.5 inserted, M3 and M8 split, and owner-gate and seam columns added. Seams are defined in [06-backbone-contract.md](06-backbone-contract.md). **Do not start a milestone whose owner gate is still Proposed.**

| M | Scope | Claims | Owner gate | Seams | Main new code |
|---|---|---|---|---|---|
| **M0** ✅ | Fixtures, evidence checker, design | — (50 subjects pinned) | — | — | `examples/fptr-*`, `p132`–`p144`, `scripts/check_fptr_evidence.py` |
| **M0.5** ✅ | Gate hardening: probes → `examples/probes/`, F36–F43, strict discovery, ctest `fptr`; verified on LLVM 23.1.4 (drift 0) with portable build fixes | — (65 subjects pinned) | D19 | — | `scripts/check_fptr_evidence.py`, `tests/CMakeLists.txt` |
| **M1** ✅ | Facet only, no semantic change: `indirectCallResolution` from today's state; `exact`/`set`@L0, `parametric` (+`parametricClosure`), `unresolved`; paper-facts schema **v9 → v10** + validator + aggregator CSV | F01, F02, F25, F31, F38, F39, F40, F41, F43:`run_local`‡, F53 (control) | D12 ✅ | S8 | `FptrResolution.h` (facet builder) |
| **M2a** | CFG infrastructure + **loops** (08 §3.4): `FptrCFGSolver` pre-pass (intra), table lookups, back edges, dispatch evidence | F10, X12, F36, F44, F45, F46:`run_while` | D24, D25, D30 | S1, S6 | `FptrCFGSolver` (new) |
| **M2b** | **conditions** (08 §3.5): if/switch/fallthrough/`?:`/`&&`/`\|\|` edge guards, equality refinement; fn-type cast labels (08 §7) | F08, F09, X11, F14, F29, F47, F56 | D29, D30 | S6 | edge-label guards, cast labels |
| **M2c** | **goto** and combinations (08 §3.5) | F50, F51 | D24 | — | indirect-goto block handling |
| **M3a** | L1 pre-pass, global slots, file-scope initializers → L1, L0 tri-state | F03, F04, F32, F34, F35, **F43** | D18 | S1, S6 (fallback), S7 | `FptrSlotCollector`, `TargetSet`, `SlotKey` (global) |
| **M3b** | array / table slots, struct arrays, `memcpy`; table dispatch correlation (D30) | F05, F06, F07, F26, F33, F54, F55 | D8 (F33 guards) | — (via S1/S6) | `SlotKey` (array) |
| **M3c** | record slots (D7′), unions, cast reads | F15, F16, F30, **F37**, **F42** | D7′ | — (via S1/S6) | `SlotKey` (record, D7′ rules) |
| **M4** | per-function fptr summaries (ret/out/slots), on-the-fly call graph, SCC fixpoint (08 §4); `namedCallEdges` for resolved variable/table/L1 targets | F11, F12, F17, F27, F52 | D26 | S9 | summaries in `FptrCFGSolver` |
| **M5** | Binding forms: fptr through `void*`, `va_arg`, return alias through indirect calls, struct `{fn,arg}` executor (equality refinement moved to M2b) | F13, F20, F24, F28 | — | S6 | higher-order binding helpers |
| **M6** | Callback–context binding | F18, F19 | D9 | — | producer/consumer detection |
| **M7** | Library model | F21, F22, F23 | D5, D6 (hint) | S6 | `LibraryCallbackModels.def` |
| **M8a** | Cross-TU global/array slots | X01, X02 (re-check X11, X12†) | D27 | — | `fptrFacts.slots`, solved in C++ import rounds (D27) |
| **M8b** | Cross-TU record slots | X03, X08 | D7′ | — | record keys in the index |
| **M8c** | Cross-TU returns / FSM (CFG-derived return summaries, D27) | X04, X07, X14 | D27 | — | `fptrFacts` summaries |
| **M8d** | Cross-TU CCB, LIB, executor | X05, X06, X09, X10, X13 | D9 | — | `fptrFacts.callbackBindings` |
| **M9** | L2 signature fallback (flag); coverage aggregation; migrate fixture checks into `FileCheck` under `test/` | — (new L2 fixtures first, as M9a) | D11 | S6 | `--fptr-signature-fallback`, lit config |

**Round 4 (CFG, 08):** M2 is re-cut into M2a/b/c, in the owner's priority order loops > conditions > goto. The AST-patch seams S2–S5 are retired. Controls already met at pin time, guarded by `@baseline`: F46 `run_do`, F48, F49, F53. F53 is claimed by M1 as a control.

‡ F43 `run_local::ctx` was already correct and needed only the facet, so it flipped in M1 as an unclaimed subject. Per the rules it moved into M1's claim (REVIEW.md, 2026-11-25 M1 entry). F43 `run::ctx` stays with M3a.

† X11/X12 are claimed by M2a/M2b (they only need L0 control flow plus today's cross-TU import). M8a re-runs them as a regression check after slot serialization; it does not claim them.

**Dependency order:** M0.5 → M1 → M2a → M2b → M2c → M3a → (M3b, M3c) → (M4, M5) → M6 → M7 → M8a–d → M9.

- M2 comes first among the semantic milestones because it fixes wrong answers (F09, F10, F36, X11, X12).
- M3a comes before M3b/M3c because it introduces the pre-pass (S1), the slot resolver (S6) and the tri-state (S7) that they reuse.
- M8 depends on M3–M7, because the cross-TU facts are the serialized form of the L1/RET/CCB facts.

**Observability (round 3).** Every fptr iteration added in M2/M4/M8 reports its termination reason in text and JSON (the engine's same-TU solve reports only in text; see G6 in 05).

Use the task-card template in [06-backbone-contract.md §5](06-backbone-contract.md#5-task-card-template-copy-into-the-pr-description) as the PR description. PR title: `<Mx>: <descriptive name>` (CLAUDE.md).

## Definition of done (per milestone)

1. Claimed fixtures meet `@expected`. The baselines are re-pinned, and the project READMEs and evidence table are regenerated.
2. `--check baseline` is clean for all other fixtures.
3. All `examples/` and `test_projects/` run without new failures (CLAUDE.md rule), and the existing p05, p12–p17 and p34–p48 fptr projects keep their expected outputs. `ctest -L fptr` and `ctest -LE known-failing` are green.
3a. Visitor changes are confined to the seams in the task card, within the diff budget ([06-backbone-contract.md](06-backbone-contract.md)).
4. The schema and validator are updated if output changed.
5. REVIEW.md has a dated entry with what changed, deviations, and any new open question.

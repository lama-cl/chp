# 04 — Incremental implementation plan

Each milestone **claims** a list of fixtures. A milestone is done when exactly its claimed fixtures switch from `xfail` to `expected-met`, and every other fixture keeps its pinned baseline.

## Agent workflow (every milestone)

```bash
# 0. build (LLVM 18 or newer with clang dev packages)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=$(llvm-config --cmakedir) \
      -DClang_DIR=$(llvm-config --libdir)/cmake/clang -G Ninja && ninja -C build
# 1. before touching code: the evidence must be green
python3 scripts/check_fptr_evidence.py --check baseline
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

| M | Scope | Claims | Main touch points |
|---|---|---|---|
| **M0** ✅ | Fixtures, evidence checker, design (this change) | — (baselines pinned for all 50 subjects) | `examples/fptr-*`, `test_projects/projects/p132`–`p144`, `scripts/check_fptr_evidence.py` |
| **M1** | Facet only, no semantic change: `indirectCallResolution` from today's state; `exact`/`set`@L0, `parametric`, `unresolved`; `signatureMismatch`; paper-facts schema v4 + validator + aggregator CSV | F01, F02, F14, F25, F31 | `FptrResolution.h` (new), `paperSubjectJson`, `schemas/cast-paper-facts.schema.json`, `scripts/validate_paper_facts.py`, `scripts/aggregate_paper_facts.py` |
| **M2** | Soundness + expression forms (D4, D8 for L0): switch join, loop pre-scan widening, `?:` | F08, F09, F10, X11, X12 | `TraverseSwitchStmt`, `Traverse{For,While,Do}Stmt`, `resolveFunctionValueSet`, `mergeFunctionPointerStates` |
| **M3** | L1 TU-level storage: pre-pass; globals, arrays, struct arrays, record-type slots, unions, `memcpy` | F03, F04, F05, F06, F07, F15, F16, F26, F30, F32, F33, F34, F35 | new pre-pass visitor, `resolveIndirectCallee`, per-function L0 seeding (`:1877`) |
| **M4** | RET/out-param target summaries, FSM fixpoint, indirect edges in the SCC | F11, F12, F17, F27 | `FunctionSummary` (new `returnTargets`), `SummaryDependencyEdge.layer`, named-call SCC |
| **M5** | Binding forms: fptr through `void*`, `va_arg`, equality refinement, return alias through indirect calls, struct `{fn,arg}` executor | F13, F20, F24, F28, F29 | `stripFunctionValueCasts`, higher-order binding, return-alias continuation |
| **M6** | Callback–context binding (D9) | F18, F19 | producer/consumer detection, derived `$b := $a.$k` summaries |
| **M7** | Library model (D5, D6) | F21, F22, F23 | `LibraryCallbackModels.def`, element relation via PtrArith attach path |
| **M8** | Cross-TU: `fptrFacts` in summaries, index merge and solve, import | X01–X10, X13 | `schemas/cast-summary.schema.json`, `scripts/build_summary_index.py`, `SummaryIndex.h`, import lookup |
| **M9** | L2 signature fallback (flag); coverage aggregation; migrate fixture checks into `FileCheck` tests under `test/` | — (new L2 fixtures added first, as M9a) | `--fptr-signature-fallback`, `scripts/aggregate_*`, lit config |

**Dependency order:** M1 → M2 → M3 → (M4, M5) → M6 → M7 → M8 → M9.

- M2 comes first among the semantic milestones because it fixes wrong answers.
- M8 depends on M3–M7, because the cross-TU facts are the serialized form of the L1/RET/CCB facts.

## Definition of done (per milestone)

1. Claimed fixtures meet `@expected`. The baselines are re-pinned, and the project READMEs and evidence table are regenerated.
2. `--check baseline` is clean for all other fixtures.
3. All `examples/` and `test_projects/` run without new failures (CLAUDE.md rule), and the existing p05, p12–p17 and p34–p48 fptr projects keep their expected outputs.
4. The schema and validator are updated if output changed.
5. REVIEW.md has a dated entry with what changed, deviations, and any new open question.

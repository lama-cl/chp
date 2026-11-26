# Function-pointer (fptr) extension

This extends Cast History so that `void*` and other data pointers flowing through **function pointers** can be classified. Resolution quality is reported in a facet (`indirectCallResolution`) next to the 7 primary categories.

## Status (2026-11-25)

| Milestone | State |
|---|---|
| M0, M0.5: evidence, gate, LLVM 23 | done. 70 fixtures, 80 subjects, drift 0 (`ctest -L fptr`) |
| M1: resolution facet (paper facts v10, seam S8) | done |
| M2a/b/c: CFG solver, loops > conditions > goto ([08](08-cfg-fptr-resolution.md)) | next; gated on D24–D28 |
| M3–M9: slots, summaries/recursion, binding, CCB, LIB, cross-TU, L2 | planned ([04](04-implementation-plan.md)) |

**Wrong answers today:** F09, F10, F35, F36, F43:run, F46:run_while, F47, F50, F51, X11, X12. All of them come from the missing CFG or from treating a global initializer as the current value.

## Key decisions

- **D1:** keep the 7 categories; add a facet.
- **D12:** facet ordering is `… < set < parametric = exact`, with `indirectHistoryComposition` and `parametricClosure`.
- **D24:** a CFG-based fptr solver as a TU pre-pass; no semantic edit to existing visitor code (seams S1, S6, S8, S9).
- **D29:** fn-type casts get the labels `fnptr-transport`, `fnptr-param-variance` (UB, flagged) and `fnptr-incompatible`.
- **D30:** dispatch correlation (`single-view`, `object-correlated`, `per-iteration-subject`, `unrelated`) decides multi-target categories.

## Read in this order

| Doc | For | Content |
|---|---|---|
| [REVIEWING.md](REVIEWING.md) | humans, start here | what to read, commands, red flags, LLVM 23 recipe |
| [REVIEW.md](REVIEW.md) | owner | discussion log for rounds 1–4, owner decisions, open questions |
| [00-challenges.md](00-challenges.md) | everyone | why fptrs are hard; measured gaps |
| [01-fact-catalog.md](01-fact-catalog.md) | everyone | 70 facts (F01–F56, X01–X14) plus documented-only N1–N10 |
| [07-existing-engine.md](07-existing-engine.md) | implementers | today's engine: no CFG, SCCs, capped fixpoints |
| [08-cfg-fptr-resolution.md](08-cfg-fptr-resolution.md) | everyone | **the design**: CFG solver, recursion, cross-TU, D29/D30 |
| [02-design.md](02-design.md) | implementers | data model, slots, facet, LIB, cross-TU formats |
| [03-decisions.md](03-decisions.md) | reviewers | ADRs D1–D30 |
| [04-implementation-plan.md](04-implementation-plan.md) | agents | milestones, claims, gates, workflow |
| [06-backbone-contract.md](06-backbone-contract.md) | agents and reviewers | seams, invariants, task card |
| [05-plan-review.md](05-plan-review.md) | history | round-2 review and options |
| [evidence/evidence-table.md](evidence/evidence-table.md) | everyone | generated: baseline vs expected |

## Fixtures

- Single TU: `examples/fptr-NN-<fact>.c`. Cross-TU: `test_projects/projects/p1NN_fptr_xtu_*`. Exploratory probes live in `examples/probes/` and are not fixtures.
- Header markers: `@fptr-fact`, `@subject`, `@baseline` (today, pinned) and `@expected` (required). Keys: category, callback, behavior, types, resolution, sigmismatch, ubkind, dispatch.

```bash
unset C_INCLUDE_PATH CPLUS_INCLUDE_PATH          # see REVIEWING.md §LLVM
(cd build && ctest -L fptr --output-on-failure)  # baseline gate
python3 scripts/check_fptr_evidence.py --check expected --only <ids>
```

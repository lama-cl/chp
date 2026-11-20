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

## 7. Change log

| Date | Change |
|---|---|
| 2026-11-18 | Probes; challenges identified; owner decisions D1, D2, D4, D5/D6, D10 |
| 2026-11-20 | M0 landed: 35 single-TU fixtures (`examples/fptr-01..35`), 13 cross-TU projects (`p132`–`p144`), `scripts/check_fptr_evidence.py`, design docs. F34/F35 (global-initializer unsoundness) found while writing the fixtures. |

# 03 — Decision log (ADR style)

Status values:
- **Accepted (owner)**: chosen by the project owner in the design discussion.
- **Accepted (derived)**: follows directly from an owner decision.
- **Proposed**: the author's recommendation; needs owner review. Open items are also listed in [REVIEW.md](REVIEW.md#open-questions-for-the-owner).

| ID | Decision | Status |
|---|---|---|
| D1 | Keep the 7 primary categories; add an orthogonal `indirectCallResolution` facet | Accepted (owner) |
| D2 | Layered resolution L0 → L1 → L2 with a precision (layer) tag on every target | Accepted (owner) |
| D3 | Globals are read through L1 slots (initializer ∪ all stores), with an L0 overlay for in-function stores | Accepted (derived from D2) |
| D4 | Fix the switch/loop unsoundness inside the fptr extension, for fptr state only; document it with fixtures | Accepted (owner) |
| D5 | Curated library callback model (libc/POSIX/C11 first) | Accepted (owner) |
| D6 | `dlsym` results are ExternalUnknown; a matching project name is only a hint | Accepted (owner); hint part Proposed |
| D7 | Record slots are keyed by record type + field path (object-insensitive) | Proposed |
| D8 | Guard policy: a selector visible at the call → guarded; temporal selection → unguarded | Proposed |
| D9 | Callback–context binding is reduced to existing higher-order `$k.$i` summaries when same-instance storage can be shown | Proposed |
| D10 | Evidence first: fixtures with pinned baselines before code; markers are the single source of truth | Accepted (owner) |
| D11 | Target-set cap K = 16; L2 off by default (flag), capped at 8 | Proposed |
| D12 | Facet distinguishes `unresolved` / `external-*` / `parametric` and records `composed`; `callbackMediation` is unchanged | Proposed |
| D13 | Fixture conventions: `examples/fptr-NN-*.c` (Fnn), `test_projects/projects/p1NN_fptr_xtu_*` (Xnn), one fact each | Accepted (owner directive) |
| D14 | L1 pre-pass runs before function traversal (order independence) | Accepted (derived from D2/D3) |
| D15 | Loops use pre-scan widening, not body re-traversal | Proposed |
| D16 | New data model lives in `FptrResolution.h`, not in `CastHistoryVisitor.h` | Proposed |

---

### D1 — Categories unchanged; resolution facet added
- **Context.** The probes showed unresolved indirect calls landing in `OpaqueTransportOrViewOnly`.
- **Options.**
  1. Keep the 7 categories and add a facet.
  2. Add an 8th category, `IndirectCallUnresolved`.
  3. Route unresolved calls to `ExternalUnknown`.
- **Decision.** Option 1.
- **Owner's rationale (verbatim):** *"OpaqueTransportOrViewOnly is just the category of a pointer in the scope of its visible history. Adding a resolution facet makes more sense that can be combined later with different categories."*
- **Consequences.** No schema-breaking change to `primaryCategory`. Paper-facts `schemaVersion` goes 3 → 4 (additive). Aggregators gain a `primaryCategory × resolution` table.

### D2 — Layered resolution with precision tags
- **Layers.**
  - **L0:** flow-sensitive local state (today's mechanism).
  - **L1:** flow-insensitive storage facts — globals, arrays, record slots, returns and out-params; TU-wide first, project-wide via the index.
  - **L2:** signature-compatible address-taken functions.
- **Rule.** Every target carries its layer, so reports can filter by confidence.
- **Rejected alternatives.** L0+L1 only (lower coverage); signature-only (large, imprecise sets).

### D3 — Globals via L1
- **Evidence.**
  - F35: the initializer is treated as the current value in every function, which is **unsound**.
  - F34: the definition placed after the use is unresolved.
  - F03 works only because of traversal order.
- **Decision.** Inside a function, a global with no store on the current path is read through its L1 slot (initializer ∪ every store in the TU, or in the project after import). A store inside the function is still a strong update for the rest of that function (the L0 overlay).

### D4 — switch/loop soundness, fptr state only
- **Owner's choice:** *"Fix in fptr ext, fptr state only; Document the examples in examples or test_projects for both cases."*
- **Fixtures.** F09, F10 (single TU) and X11, X12 (cross-TU) pin today's wrong answers (`double*` only, `int*` only).
- **Out of scope.** The same syntactic traversal affects origin, field and alias state. That needs a separate issue; see REVIEW concern C4.

### D5 — Library callback model
- **Decision.** A data table (`LibraryCallbackModels.def`) with `fnArg`, `dataArg`, `targetParams` and `relation ∈ {direct, element}`.
- **Scope.** libc/POSIX/C11 first; GLib/libuv later (N5).
- **Guard.** A row applies only when the callee has no project definition, so project-defined `qsort` wins (`examples/qsort.c`).

### D6 — dlsym
- **Owner's choice:** "library model + dlsym unknown".
- **Decision.** A value from `dlsym` makes the subject `ExternalUnknown` with `resolution=external-unknown@LIB`. The loaded object is not part of the analyzed build.
- **Proposed addition.** If the symbol string is a literal that matches a project function, report it as a `nameHint`, but never compose it (X10).

### D7 — Object-insensitive record slots (Proposed)
- **Rationale.** This matches how C vtables are used (one ops type, few instances). It is cheap, and it solves F15, F16, F30, X03 and X08.
- **Risk.** Over-merging in code bases that reuse one generic ops type for unrelated purposes. Mitigations:
  - the K cap;
  - the L0 per-object overlay;
  - the facet shows `L1`, so aggregates can separate it.

### D8 — Guard policy (Proposed, affects category counts)
- **Guarded** when the selector is a value at the call: a branch or `?:` condition, a switch case, a table index, or the object's own vtable slot. These sets become `GuardedActiveReinterpretation`.
- **Unguarded** when the selection is temporal: a loop back edge, or a global mutated elsewhere. These sets become `UnguardedMultiTargetActiveReinterpretation`.
- **Fixtures.** F08, F09, F33 (guarded); F10, F35 (unguarded).
- **Alternatives.**
  1. Everything guarded, because each call picks exactly one target. This hides real reinterpretation of the same object across calls.
  2. Everything unguarded. This over-reports tagged-dispatch designs.

### D9 — Callback–context binding (Proposed)
- **Rule.** A producer that stores `(fn = $a, arg = $b)` into the same instance, plus a consumer that calls `X.fn(X.arg)` on that record type, gives the producer the derived summary `$b := $a.$k`.
- **Effect.** Registration sites specialize exactly like direct higher-order calls, which keeps pairs correlated (F19, X06).
- **Weak case.** If the same instance cannot be shown, no binding is derived; the record-slot cross product is used (sound, imprecise) and the facet reports `binding=weak`.

### D10 — Evidence first (Accepted)
- **Owner's directive:** *"`examples` and `test_projects` should have test evidence before new changes are made to code. All tests should be self contained and be incremental units as much as possible."*
- **Implementation.**
  - Each fixture has `@fptr-fact`, `@subject`, `@baseline` (observed today) and `@expected` markers.
  - `scripts/check_fptr_evidence.py --check baseline` guards against drift.
  - `--check expected --only <ids>` is the acceptance test for each milestone.
  - The generated table is [evidence/evidence-table.md](evidence/evidence-table.md).

### D11 — Caps (Proposed)
- **K = 16** per target set. Above the cap the set becomes `⊤(cap)` → `unresolved`, with the reason recorded.
- **L2** is behind `--fptr-signature-fallback` (default off), with at most 8 candidates.

### D12 — Facet semantics (Proposed)
- **States** are ordered `unresolved < external-unknown < signature < parametric < external-modeled < set < exact`. A subject reports its weakest site. Per-site `composed` records whether any summary was actually transferred.
- **Open question.** `callbackMediation=Resolved*` is currently set without a transfer (F07, F13, F20, F24, F27, F28). Proposed: leave it unchanged for compatibility, and add a validator warning when `Resolved*` appears with `composed=false`.

### D13 — Fixture conventions (Accepted)
- One fact per fixture. Read-only handlers (`return *q`), so multi-target outcomes are not masked by the incompatible-writes rule.
- IDs: `F01`–`F35` single TU, `X01`–`X13` cross-TU, `N1`–`N9` documented only.
- Cross-TU projects use numbers `p132`–`p144`, after the existing `p131`.

### D14 — L1 pre-pass
- A collection pass over the whole TU runs before the main traversal. L1 facts therefore do not depend on declaration order (F34).

### D15 — Loop pre-scan widening (Proposed)
- **Rule.** Seed the loop-head state with the entry state ∪ every fptr store in the body; traverse once.
- **Alternative rejected.** A two-pass body traversal duplicates cast observations and pointer-use records.

### D16 — Code placement (Proposed)
- **Location.** New types (`SlotKey`, `TargetSet`, `IndirectCallResolution`) and the resolver go in `include/CastHistory/FptrResolution.h`. The library table goes in `LibraryCallbackModels.def`. Both use LLVM containers (`SmallVector`, `DenseMap`) per CLAUDE.md.

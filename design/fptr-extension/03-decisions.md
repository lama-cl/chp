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
| D7 | Record slots are keyed by record type + field path (object-insensitive) | Superseded by D7′ |
| D8 | Guard policy: a selector visible at the call → guarded; temporal selection → unguarded | Superseded by D30 |
| D9 | Callback–context binding is reduced to existing higher-order `$k.$i` summaries when same-instance storage can be shown | Proposed |
| D10 | Evidence first: fixtures with pinned baselines before code; markers are the single source of truth | Accepted (owner) |
| D11 | Target-set cap K = 16; L2 off by default (flag), capped at 8 | Proposed |
| D12 | Facet distinguishes `unresolved` / `external-*` / `parametric` and records `indirectHistoryComposition`; `callbackMediation` is unchanged | Accepted (owner, amended 2026-11-25) |
| D13 | Fixture conventions: `examples/fptr-NN-*.c` (Fnn), `test_projects/projects/p1NN_fptr_xtu_*` (Xnn), one fact each | Accepted (owner directive) |
| D14 | L1 pre-pass runs before function traversal (order independence) | Accepted (derived from D2/D3) |
| D15 | Loops use pre-scan widening, not body re-traversal | Superseded by D24 |
| D16 | New data model lives in `FptrResolution.h`, not in `CastHistoryVisitor.h` | Proposed |
| D7′ | Record slots keyed by the **outermost record visible in the access path**; cast reads union prefix-compatible slots or give ⊤ (supersedes D7) | Proposed (round 2) |
| D17 | Seam architecture: backbone changes only at seams S1–S8 ([06-backbone-contract.md](06-backbone-contract.md)) | Proposed (round 2); the owner asked to preserve the backbone |
| D18 | L0 tri-state `known / killed-unknown / no-info`; L1 is consulted only on `no-info` | Proposed (round 2) |
| D19 | Gate hardening: probes in `examples/probes/`, strict fixture discovery, ctest label `fptr`, LLVM pin in `examples/fptr-meta.info`, fptr fixtures added to the `regress.py` set at M1 | Accepted (owner) for the probe move; the rest Proposed |
| D20 | Split M3 → M3a/b/c and M8 → M8a–d; each milestone carries an owner gate | Proposed (round 2) |
| D21 | Target sets have a deterministic order (lookup key, then source location) | Proposed (round 2) |
| D22 | Cross-TU L1 slot solving runs in C++ during import rounds (next to the cross-TU two-phase solve), not as the first solver in `build_summary_index.py` | Resolved by D27 |
| D23 | Early exits: for fptr state, a branch ending in `return`/`break`/`continue` contributes nothing to the join (M2, seam S5); a full CFG stays option (c) | Superseded by D24 |
| D24 | CFG-based fptr solver (`clang::CFG`, TU pre-pass) supersedes the AST patches S2–S5, D15 prescan and D23; fptr only (D4) | Proposed (round 4; owner asked for the CFG) |
| D25 | Guards come from CFG edge labels and are evidence, not the classifier; back edges marked; guard cap | Proposed (round 4) |
| D26 | TU solver: per-function fptr summaries, on-the-fly call graph, capped SCC fixpoint; termination in text and JSON | Proposed (round 4) |
| D27 | fptr summaries serialized cross-TU; index merges only; C++ import rounds solve; driver gains an fptr fingerprint | Proposed (round 4) |
| D28 | No-CFG fallback to the AST L0 maps plus `--fptr-cfg-crosscheck` | Proposed (round 4) |
| D29 | fn-type casts get a cast history: `fnptr-transport` (no flag), `fnptr-param-variance` (flag, `ubKind=param-type-incompatible`), `fnptr-incompatible` (flag, `ubKind=signature-incompatible`) | Accepted (owner, round 4) |
| D30 | Dispatch correlation replaces the D8 guard policy: `single-view` / `object-correlated` / `per-iteration-subject` / `unrelated` decide the category | Accepted (owner, round 4) |

---

### D1 — Categories unchanged; resolution facet added
- **Context.** The probes showed unresolved indirect calls landing in `OpaqueTransportOrViewOnly`.
- **Options.**
  1. Keep the 7 categories and add a facet.
  2. Add an 8th category, `IndirectCallUnresolved`.
  3. Route unresolved calls to `ExternalUnknown`.
- **Decision.** Option 1.
- **Owner's rationale (verbatim):** *"OpaqueTransportOrViewOnly is just the category of a pointer in the scope of its visible history. Adding a resolution facet makes more sense that can be combined later with different categories."*
- **Consequences.** No schema-breaking change to `primaryCategory`. Paper-facts `schemaVersion` goes 9 → 10 (additive; the "3 → 4" written at M0 was stale). Aggregators gain a `primaryCategory × resolution` table.

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

### D12 — Facet semantics (Accepted by owner 2026-11-25, amended)
- **Owner amendment.** The states are ordered `unresolved < external-unknown < signature < external-modeled < set < parametric = exact`. A subject reports its weakest site; on a `parametric`/`exact` tie it reports `parametric`, because that carries the deferral information.
- **Owner amendment (rename).** The per-site field `composed` is renamed **`indirectHistoryComposition`**, with values `full` (every target's summary was transferred), `partial` (some were) and `none`. This is a refinement of the boolean the owner proposed, so that a multi-target set with a missing summary is visible.
- **Author's pushback, recorded with the owner's request:** `parametric = exact` holds only for a **closed** formal, where every call site in the analyzed scope passes a resolvable function (for example `hof(fp, a)` with `fp = use_int`). An **open** formal, such as an exported `int apply(handler_t cb, void *ctx) { return cb(ctx); }` with no callers, has the same summary (`ctx => $0.$0`), but no type is ever determined. Ranking it like `exact` would overstate coverage. **Resolution:** keep the owner's ordering and add the per-site field `parametricClosure: closed | open`. M1 computes it within the TU; M8 recomputes it project-wide. Aggregates report `parametric(open)` separately.
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
- **Amended (round 3).** Any fptr iteration (prescan RHS resolution, F17 loop-carried returns) follows the engine's existing fixpoint convention ([07 §3](07-existing-engine.md#3-inter-procedural-sccs-and-fixpoints)): cap `MaxNamedCallSummaryIters` (16), a termination reason in text **and** JSON, and an `Approximated` feature on hitting the cap.

### D16 — Code placement (Proposed)
- **Location.** New types (`SlotKey`, `TargetSet`, `IndirectCallResolution`) and the resolver go in `include/CastHistory/FptrResolution.h`. The library table goes in `LibraryCallbackModels.def`. Both use LLVM containers (`SmallVector`, `DenseMap`) per CLAUDE.md.

---

## Round 2 (2026-11-25), see [05-plan-review.md](05-plan-review.md)

### D7′ — Record slot keys: outermost visible path (Proposed, supersedes D7)
- **Evidence.** F37: keying by the innermost type (`record:struct ops.fn`) merges two outer records into a false multi-type answer. F42: a read through `(struct base *)` hits a type with no stores.
- **Rule.**
  1. The key is the outermost record type visible in the member-access chain plus the field path: `x->o.fn` → `record:struct X.o.fn`.
  2. An access that starts from a pointer to the inner type (`struct ops *p; p->fn`) uses `record:struct ops.fn`. That slot is the union of every outer slot that embeds `struct ops`.
  3. A read through a pointer that was cast from another record type (F42) unions every slot of records whose leading members are layout-compatible with the read path. If that set cannot be computed, the result is `⊤(cast)` → `unresolved`, never empty.
- **Cost.** Rules 2 and 3 widen sets in GObject-style code. Mitigated by the K cap and the `L1` layer tag. Measure on bench-3c at M3c.

### D17 — Seam architecture (Proposed)
- **Context.** The owner asked to preserve the visitor backbone and modify it only when strictly necessary. The visitor is 32 128 lines.
- **Decision.** New logic lives in `FptrResolution.h`. The visitor is touched only at seams S1–S8, within a diff budget. S5 (the if-merge) is the only semantic edit to existing code, because it fixes an existing defect (F36).
- **Alternatives.** Growing the visitor in place (rejected: not reviewable); a CFG-based fptr dataflow (kept as a fallback for switches); a summary post-pass (used only for cross-TU).

### D18 — L0 tri-state (Proposed)
- **Evidence.** F43 `run_local`: `g = lookup_plugin(..)` erases the L0 binding. If the resolver then fell back to L1, it would return the initializer target `use_int`, which is unsound.
- **Decision.** L0 distinguishes `known(set)`, `killed-unknown` (a strong update to an unresolvable value: report `unresolved`) and `no-info` (never written in this function: consult L1).

### D19 — Gate hardening (probe move Accepted by the owner; rest Proposed)
- The owner's exploratory probes live in `examples/probes/` (byte-identical) and are not fixtures.
- `check_fptr_evidence.py` only accepts `examples/fptr-NN-*.c` and names any other `fptr-*.c` file.
- ctest test `fptr-baseline` (label `fptr`).
- The baselines are re-pinned on the project's LLVM (23 on the owner's host) in a dedicated commit, with the version in `examples/fptr-meta.info`.
- At M1, the fptr fixtures join the `regress.py` full-output set, so text drift is visible.

### D20 — Smaller milestones with owner gates (Proposed)
- M3 → M3a (globals, tri-state, file-scope initializers), M3b (arrays and tables), M3c (record slots, D7′).
- M8 → M8a (global/array slots), M8b (record slots), M8c (return/FSM), M8d (CCB, LIB, executor).
- Every milestone names the decision it depends on. An agent stops if that decision is still Proposed.

### D21 — Deterministic target order (Proposed)
- `std::set<const FunctionDecl*>` orders by address. New `TargetSet`s order by (lookup key, file, line, column). Existing L0 sets are converted at the seam.

## Round 3 (2026-11-25), see [07-existing-engine.md](07-existing-engine.md)

### D22 — Where cross-TU slots are solved (Proposed)
- **Fact.** `build_summary_index.py` builds SCC metadata only ("Python does not execute the C++ SCC solver"). All fixpoints run in C++ (same-TU and cross-TU two-phase) or in the external driver over import rounds. `SUMMARY_IMPORT_ROUNDS` defaults to 1.
- **Options.** (a) Solve `slot ⊇ param:f.$k` constraints in Python while building the index: simple, but it is the index's first solver and duplicates convergence/caps logic. (b) Serialize slot facts; let the C++ import round solve them with the existing cap and termination conventions and report status in the existing cross-TU fixpoint JSON.
- **Recommendation.** (b): one solver family, one convergence report. Forwarding chains then need either rounds ≥ 2 or the external driver; X fixtures already run with 2.

### D23 — Early exits for fptr state (Proposed)
- **Evidence.** F44: the engine has no CFG and `return`/`break`/`continue` do not end a branch's state, so infeasible targets reach later calls (imprecise, not unsound).
- **Decision.** In the S5 merge, a then/else branch whose last statement is a `return`, `break` or `continue` (syntactically, including in nested compound blocks) contributes no fptr state to the join. Fptr state only (D4). `goto` stays unsupported.
- **Alternative.** Option (c), a `clang::CFG` for fptr state, fixes this together with switch fallthrough, loops and `goto`, at the cost of the engine's first CFG (`clangAnalysis` dependency, block↔guard-text mapping). Trigger to switch: if S2–S5 exceed their diff budget, or bench runs show a high `Approximated` rate for fptr state.

## Round 4 (2026-11-25), see [08-cfg-fptr-resolution.md](08-cfg-fptr-resolution.md)

### D24 — CFG-based fptr solver (Proposed)
- **Owner request.** Add a CFG supporting loops > conditions > goto and their combinations, used for fptr resolution, with recursion and cross-TU composition.
- **Evidence.** Unsound today: F09, F10, F36, F46, F47, F50, F51, X11, X12. Imprecise: F44, F45. Bench C code has 9,204 `goto` statements in 732 files.
- **Decision.** A `clang::CFG` worklist solver runs as a TU pre-pass (seam S1). The visitor consults its table (S6). There is no semantic edit to existing visitor code. It covers fptr state only (D4).
- **Supersedes** S2–S5 and S7, D15 and D23.

### D25 — Edge-labelled guards (Proposed)
- Guards come from CFG terminators (`[c]`, `[!(c)]`, `[k == c]`), rendered with `exprToString`. Back edges are marked. Guards over the cap are dropped, with the reason. Guards are evidence for D30, not the category classifier.

### D26 — TU summaries and an on-the-fly call graph (Proposed)
- The per-function summary holds `ret`, out-param effects, slot contributions and indirect sites. Tarjan runs bottom-up, with a capped SCC fixpoint (16) and capped re-planning (16). Termination goes to text and JSON (G6). The cast-history recursion reuses the existing solves via S9 edges.

### D27 — Cross-TU fptr summaries (Proposed; resolves D22)
- `fptrFacts` are serialized. `build_summary_index.py` only merges. The C++ import round solves with the same caps. `run_scc_param_fixpoint.py` gains an fptr fingerprint. Rounds = 1 is documented as insufficient for forwarding chains.

### D28 — Fallback and cross-check (Proposed)
- When no CFG can be built, the AST L0 maps are used and `Approximated` is recorded. `--fptr-cfg-crosscheck` reports where the CFG and AST results differ.

### D29 — Function-type casts (Accepted by owner)
- **Owner.** *"Case A (transport) and case B (pointer-parameter variance) can be labelled similar to cast history labels for pointers. Case B being UB is more dangerous than case A and should be flagged with reason to support later analyses that may cover UB conditions or multiple platforms."*
- **Labels:**
  - `fnptr-transport`: no flag. F53, F13.
  - `fnptr-param-variance`: `signatureMismatch=true`, `ubKind=param-type-incompatible`, plus argument cast evidence. F14.
  - `fnptr-incompatible`: `ubKind=signature-incompatible`. F56.

### D30 — Dispatch correlation (Accepted by owner, replaces D8)
- **Owner.** Loops are modelled well but are not well defined for history, and "unguarded" is not an interesting tag for fptrs. Table dispatch is safe when reads and writes happen on the same object through the same function. Use the subject and function set to decide mismatch.
- **Rule** ([08 §6](08-cfg-fptr-resolution.md#6-what-a-target-set-means-for-cast-history-dispatch-correlation-d30)):
  - `single-view` gives Monomorphic;
  - `object-correlated` gives Guarded;
  - `per-iteration-subject` is decided per call;
  - `unrelated` gives UnguardedMultiTarget.
- **Author's pushback, recorded.** Guards stay as the evidence for object correlation, for example `switch (o->tag)`.

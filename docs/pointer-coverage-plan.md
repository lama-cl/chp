# Extending cast-history pointer coverage

Status: plan, nothing implemented. Gates are in place and red.
Hooks: `gates/pointer-coverage/` (fixtures, `gates.json`, `run_gates.py`,
`coverage_ledger.py`, `baseline.json`) and CTest (`pointer-coverage-*`).

## 1. Where coverage is lost today

A pointer is "covered" when the concrete interpretations made anywhere along
its flow reach its own history. Measured on the unmodified tool, coverage is
lost at four kinds of boundary. Each row is reproduced by a fixture.

| Boundary | What the tool does now | Code anchor (`include/CastHistory/CastHistoryVisitor.h`) | Fixture |
|---|---|---|---|
| Call through fptr **array** | `unsupported: function-pointer-array-call`; data args become `UnresolvedCallback` | `recordIndirectCallFeature` :30717, `resolveFunctionValueSet` :25456 (no `ArraySubscriptExpr` case) | `fp_array_dispatch.c` |
| Call through fptr **field via pointer base** (`s->ops->area`) | `unsupported: function-pointer-field-call`; `FieldCallback` | `entityOfMemberExprNoSideEffect` :26133 keys a field on a *named instance* (`decl` + path), so a pointer base has no entity | `fp_vtable_oop.c`, `fp_fsm.c` |
| Fptr in a **global**, called in another function | `unsupported: function-pointer-variable-call` | `functionPointerTargets` :24088 is flow-sensitive per function body | `fp_callback_registered.c` |
| **Data pointer** stored in a global / heap struct, read elsewhere | store side stays `OpaqueTransport`; only the load side is typed | same instance keying; no type-keyed slot | `st_heap_carrier.c`, `fp_callback_registered.c` |
| **Union** members | each member is an unrelated field path; no overlap, no pun event, no tag guard | no union model at all (`grep -i union` finds only set-union helpers) | `un_tagged.c`, `un_type_punning.c` |
| **External** callee | `ExternalUnknown`, `externalEscape=true`, no callee name, no model | `VisitCallExpr` :5704 (`knownInternalDirect`), `classifyCallUse` :30148 knows only `mem*` names | `ext_*.c` |
| **dlsym** | result is an unresolved fptr variable | no handling | `fp_dlsym_plugin/` |

Size of the problem on a real project (vsftpd-3.0.3, 39 TUs, C1 only, so
"external" still includes other project TUs; rerun with the multi-TU pipeline
for the true number):

| pointer kind | total | resolved | blocked | opaque |
|---|---|---|---|---|
| `void *` | 62 | 17 | 15 (13 external, 2 callback) | 30 |
| other data pointers | 624 | 83 | 307 (all external) | 234 |
| function pointers | 15 | 0 | 3 | 12 |

Two things follow. The external boundary dominates by count, so P0 (naming
every loss) and P4 (models) move the headline number most. The `opaque` bucket
is where unmodelled stores hide (a pointer stored into a struct and never
typed locally looks "opaque", not "blocked"), so P2 is measured by `opaque`
shrinking, not by `blocked`.

## 2. Strategies

### 2.1 Function pointers: a function-value points-to layer

Every case in scope (callbacks, tables, vtables, FSMs) is the same question:
*which functions can this callable expression denote?* The tool answers it
only for a local variable with a dominating assignment, or a field of a named
instance. Options, from most to least precise:

1. **Flow-sensitive local map** (exists). Keep as the top tier.
2. **Slot-based, flow-insensitive function-value points-to** (recommended
   core). A *slot* is an abstract location: global variable, array (elements
   collapsed), `(record type, field path)`, function return, function-pointer
   parameter. Every address-taken function flows into slots through
   initialisers (including designated and nested `InitListExpr`), assignments,
   argument passing and returns; slot-to-slot copies are inclusion edges.
   Solve to a fixpoint per TU, export slot facts in the link summary, and
   union them in `build_summary_index.py` so a table defined in one TU
   resolves a call in another. This is Andersen-style inclusion restricted to
   function values, so it stays small.
3. **Multi-layer type analysis (MLTA)** as fallback when a slot is empty or
   polluted: candidates are address-taken functions stored into the same
   `(record type, field)` anywhere in the project.
4. **Signature match** over address-taken functions as last resort.

Recommendation: implement 2, fall back to 3, and use 4 **only for
accounting**, never for history composition. This tool is a classifier:
an over-approximated target set does not just lose precision, it fabricates
parallel histories and pushes pointers toward "wild". So every resolved call
carries a `resolutionTier` (`flow-sensitive`, `slot`, `mlta`, `signature`),
and composition consumes tiers `flow-sensitive` and `slot` (and `mlta` behind
an option).

Consumption needs no new machinery: a resolved target set feeds the existing
multi-target path (`recordMultiTargetFunctionPointerSummaryDependencies`
:1718, `recordCrossTUFunctionPointerTargetSetLookup` :5377, guarded parallel
arms). The work is in *producing* the set at the three sites that give up
today.

Per pattern:

- **Callbacks registered then fired**: slot = the global or the
  `(type, field)` the registration function stores into; the fptr parameter of
  the registration function is itself a slot fed by call-site actuals.
- **Fptr arrays / FSM tables**: slot = the array; `table[i].step` is
  `(struct transition, step)`. If the index is a constant, pick the element
  (tier `flow-sensitive`); otherwise the whole slot.
- **OOP vtables**: `(struct shape_ops, area)` fed by every `shape_ops`
  initialiser. Optional refinement: correlate with the object's `ops`
  assignment when the receiver's concrete struct is known at the call.
- **Self-argument correlation**: for `s->ops->f(s)` the downcast inside each
  target is to the embedding struct. Record the arm per target so the caller's
  history is a guarded parallel, not a flat union.

### 2.2 Data pointers hidden in structs and globals

Same slot idea for data: one abstract location per global and per
`(record type, field path)`, with a store set and a load set. A store's source
entity inherits the interpretations observed on loads of the same slot, tagged
`edgeKind: slot-store-load` and tier `slot`. The existing instance-keyed
`FieldEntity` stays as the precise tier and wins when it applies.

Options considered: allocation-site heap objects (more precise, needs alias
analysis the tool does not have) and full field-sensitive Andersen (too heavy
for an AST-level tool). Type-keyed slots are the cheapest step that covers
linked structures, context structs and registries; generic containers
(`list->data`) will merge unrelated payloads, which is honest: such a slot
*is* polymorphic, and should be reported as a polymorphic carrier with its
arm count rather than resolved to one type.

### 2.3 Unions

A union is storage overlap, which the path-per-member entity model cannot
express. Model the union object as one slot with typed *views*:

- **Type punning**: write through member A, read through member B is a
  reinterpretation `A => B` with no `CastExpr`. Emit it as a cast event of
  kind `union-pun` so it enters histories and metrics like an explicit cast,
  and set `unionMediation.kind = "type-punning"`. Pointer/integer overlap sets
  the existing `refinementFacts.integerPointerFlow`.
- **Tagged union**: detect a discriminant (sibling scalar field of the
  enclosing struct, tested by the `if`/`switch` dominating the member read).
  Reads become guarded arms; the stored pointer gets a guarded parallel
  history and `unionMediation.kind = "tagged-union"`. This connects to the
  existing `BranchTaggedUnionLike` family, which today is inferred only from
  branch-guarded casts of a `void *`.
- Out of scope for the first cut, but accounted: unions of structs sharing a
  common initial sequence, transparent unions, punning through `char[]`
  members (treat as byte view).

### 2.4 External code

Four complementary answers; the plan uses all of them, in this order.

1. **Account for it** (P0). Every escaping subject gets
   `escape: {reason, callees, site}`. Nothing is inferred, but every loss
   becomes countable by callee, which tells us which models are worth writing.
2. **Declarative contracts** (P4). A data file, not C++: per function,
   `copies` (memcpy-like pointee copy), `returnsAlias` (realloc, `strchr`),
   `invokes` (which argument is called, and which arguments feed which
   callback parameters: `qsort` base elements into `cmp.$0/$1`,
   `pthread_create` `arg` into `start.$0`), `noCapture`, `frees`. Contracts are
   compiled into synthetic summaries so they flow through the existing
   direct-call import path. Start from the top callees in the P0 ledger.
3. **Analyse the source when it exists**. Vendored libraries become TUs of the
   project index; no model needed.
4. **Leave the rest blocked, by name.** Unknown stays unknown.

### 2.5 Dynamic loading

`dlsym(h, "literal")` is a link edge by name: look the literal up in the
project summary index. Unique hit: bind the fptr with tier `name-match`.
Non-literal or no hit: `escape.reason = "dynamic-symbol"`. Plugin structs
(`dlsym` returning a pointer to an ops table) fall out of 2.1 once the symbol
is bound. `dlopen` paths are ignored: the index is the universe.

### 2.6 Validation oracle (optional, recommended for P6)

Static target sets can be checked against reality: build a benchmark with
indirect-call value profiling (`-fprofile-instr-generate` records indirect
call targets), run its test suite, and require *observed targets ⊆ resolved
targets* for every call site resolved at tier `slot` or better. This is the
only gate that measures recall on real code rather than on fixtures.

## 3. Plan

Phases are ordered by dependency. P1 and P3 and P4 are independent of each
other once P0 lands, so they can be given to separate agents; P2 shares the
slot table with P1 and should follow it; P5 needs P1.

| Phase | Deliverable | Depends on | Gates |
|---|---|---|---|
| **P0** Accounting | `escape{reason,callees,site}` on paper-facts subjects; indirect-call-site inventory (callee expr kind, tier, target count) in the report; ledger baseline per benchmark project | none | `P0-escape-accounting` |
| **P1** Function-value slots | slot table + inclusion solver; `ArraySubscriptExpr` and pointer-base `MemberExpr` cases in `resolveFunctionValueSet`; global fptr variables; `resolutionTier`; slot facts in link summary and index | P0 | `P1-fp-array-dispatch`, `P1-fp-vtable-oop`, `P1-fp-fsm`, `P1-fp-callback-registered-target` |
| **P2** Data slots | store/load sets for globals and `(type, field)`; store source inherits load interpretations; polymorphic-carrier reporting | P1 | `P2-fp-callback-registered-context`, `P2-st-heap-carrier` |
| **P3** Unions | union slot with views; `union-pun` cast events; discriminant detection; `unionMediation` | P0 | `P3-un-type-punning`, `P3-un-tagged` |
| **P4** External contracts | contract file + schema; synthetic summaries; first models chosen from the P0 ledger | P0 | `P4-ext-libc-callbacks`, `P4-ext-libc-dataflow` |
| **P5** Dynamic loading | `dlsym` literal binding via index; `dynamic-symbol` boundary | P1 | `P5-fp-dlsym-plugin` |
| **P6** Corpus | ledger ratchet on bench projects; precision audit; optional profiling oracle | all | corpus gate (section 4.3) |

Schema additions introduced by the gates (add to
`schemas/cast-paper-facts.schema.json`, bump `schemaVersion`):

- `subject.escape`: `{reason, callees[], site}`; `reason` in
  `external-unmodeled`, `dynamic-symbol`, `callback-unresolved`, ...
- `subject.unionMediation`: `{kind}`; `kind` in `type-punning`,
  `tagged-union`.
- `coverage_ledger.py` already prefers `escape.reason` over its heuristics, so
  new reasons show up as new `blocked:<reason>` buckets without changing it.

## 4. Gates

### 4.1 Fixture gates (per phase)

```bash
gates/pointer-coverage/run_gates.py --tool build/cast-history-tool
```

```bash
gates/pointer-coverage/run_gates.py --tool build/cast-history-tool --gate P1-fp-array-dispatch -v
```

Each gate has **control** checks (true today, must stay true: handler
parameters keep their types, decoy functions stay out) and **target** checks
(the phase's deliverable). A gate is green when all checks pass. Failures
print expected against actual, for example:

```text
[FAIL] target  dispatch::payload; has int *, double *; callback resolved
         -> missing types ['double *', 'int *'] (has [])
         -> callbackMediation.state is still 'UnresolvedCallback'
```

Baseline, recorded in `baseline.json`: 0 of 12 gates green, all 16 control
checks passing, all 21 target checks failing.

Precision is gated too: `fp_array_dispatch.c` contains `h_unrelated`, same
signature, address-taken, never in the table. `dispatch::payload` must not
acquire `long *`. A signature-only resolver fails this control.

### 4.2 Ratchet and phase sealing

- `ctest -R pointer-coverage-ratchet` fails if any check that passes in
  `baseline.json` fails now. It is green today and must stay green in every
  change.
- When a phase's gates turn green: run `run_gates.py --update-baseline`
  (refuses if anything regressed), and append the phase to
  `POINTER_COVERAGE_SEALED_PHASES` in `CMakeLists.txt`. From then on
  `ctest -R pointer-coverage-phase-<P>` requires the whole phase green.
- A target check may be edited only to rename an output field, in the same
  change that defines the field in the schema. Weakening an expectation (fewer
  required types, dropping a decoy) is not an acceptable way to pass.

### 4.3 Corpus gate (P0 baseline, enforced from P1 on)

```bash
PAPER_FACTS=1 scripts/run_all_multitu_projects.sh
```

```bash
gates/pointer-coverage/coverage_ledger.py --root test-output/multitu/<project> --json ledger.json --compare gates/pointer-coverage/ledgers/<project>.json
```

`--compare` fails if, for any pointer kind, `resolved` dropped or `blocked`
grew. P0 commits one ledger per benchmark project under
`gates/pointer-coverage/ledgers/`; later phases refresh them only upward.
Phase exit criteria on the corpus:

| Phase | Corpus criterion |
|---|---|
| P0 | every `blocked:*` subject has `escape.reason`; ledgers committed |
| P1 | `blocked:callback-unresolved` + `blocked:callback-field` fall on every project with indirect calls; no project's `unsupported` function-pointer feature count rises |
| P2 | `opaque-no-evidence` for `void *` falls; resolved count rises |
| P3 | every union with a pointer member in the prevalence inventory has a `unionMediation` verdict or an explicit unsupported record |
| P4 | `blocked:external-unmodeled` falls by the share attributed to the modelled callees in the P0 ledger |
| P6 | existing validators still pass (`validate_paper_facts.py`, `validate_classification_labels.py`, `validate_cast_text_json_consistency.py`); runtime per project within 1.5x of the P0 timing sidecars |

### 4.4 What a gate does not prove

Fixture gates prove the mechanism on one clean instance per pattern. They do
not prove recall on real code (section 2.6 does) and they do not bound
precision loss from type-keyed slots on generic containers (the P2
polymorphic-carrier count on the corpus does). Treat a green phase as
"mechanism present", and the corpus criteria as "mechanism useful".

## 5. Open decisions

1. **MLTA tier in composition**: default off (recommended) or on. Off keeps
   current classifications stable; on raises coverage on large codebases at
   some precision cost.
2. **Contract file format and location**: JSON under `models/` validated by a
   schema (recommended), or attributes in a C header parsed by the tool.
3. **Existing `examples/` expectations**: P1 and P2 will legitimately change
   some outputs (for instance `CallbackDependent` becoming resolved). Decide
   whether those are re-baselined per phase or kept behind a
   `--pointer-coverage-level` option until P6.

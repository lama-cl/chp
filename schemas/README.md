# Cast-history JSON schemas (C6g5f4d)

This directory defines the stable JSON contract for the C6g5f4 classification layer.

## C6g5e link/report split

C6g5e separates the translation-unit artifacts used for cross-TU analysis from
human-facing diagnostics:

- `*.cast-summary.json` has schema name `cast-history-tu-link-summary`. It is a
  compact, structured-IR interchange artifact consumed by
  `build_summary_index.py` and cross-TU import. It omits rendered histories,
  metrics, and reporting views.
- `*.cast-report.json` has schema name `cast-history-tu-report`. It contains
  the verbose diagnostic, validation, metric, and classification sections. It
  is never scanned by the C2 index builder.

The runners emit report sidecars only when `SUMMARY_REPORTS=1`. The normal
multi-TU output layout is therefore:

```text
project/summaries/*.cast-summary.json   # compact C2/import input
project/reports/*.cast-report.json      # optional human/validation evidence
```

Use the appropriate consumer for each artifact:

```bash
python3 scripts/build_summary_index.py test-output/project/summaries
python3 scripts/validate_json_summaries.py \
  test-output/project/summaries test-output/project/reports
python3 scripts/validate_cast_text_json_consistency.py \
  --root test-output/project
```

The schema is intentionally forward-compatible: verbose/debug sections may add new
fields, but the C6f-facing summaries should preserve the shape of:

- `semanticMediation`
- `stableClassification`
- `stableParamClassification`
- `paperWildness`

Recommended checks:

```bash
python3 scripts/validate_cast_summary_schema.py --root test-output/multitu
python3 scripts/validate_classification_labels.py --root test-output/multitu
```

The schema checks field shape and JSON types.  The label validator checks the
semantic vocabulary and cross-field contradictions, such as `external-direct-call`
with `externalBoundary=false`.


## Compact paper-facts sidecars (C6g5f)

`cast-paper-facts.schema.json` describes the optional
`*.cast-paper-facts.json` sidecar emitted by:

```bash
cast-history-tool \
  --paper-facts-dir test-output/project/paper-facts \
  -p build source.c
```

It is intentionally separate from both C6g5e artifacts:

```text
summaries/*.cast-summary.json       # C2/index/import only
reports/*.cast-report.json          # verbose diagnostics and validation
paper-facts/*.cast-paper-facts.json # compact paper aggregation/alignment facts
```

Each paper-facts sidecar contains stable source-oriented subject identifiers,
compact final-history metrics, operation and mediation facts, primary
operation-informed categories, and callback-use relation records. It excludes
rendered histories and structured C2 import IR. Aggregate it with:

```bash
python3 scripts/aggregate_paper_facts.py \
  --root test-output/multitu \
  --out-dir test-output/paper-facts-tables
```

The aggregator emits the history and pointer-operation tables, auditable
subject/callback CSV and JSONL rows, plus blank 3C and GenC2Rust alignment
templates. C6g5f2 additionally introduces `schemaVersion: 2` paper facts with
AST-derived `declaredType.outerPointerKind`, plus the dedicated aggregation
script:

```bash
python3 scripts/aggregate_declared_pointer_populations.py \
  --root test-output/multitu \
  --out-dir test-output/declared-pointer-populations \
  --strict
```

It writes the two final-history local/parameter corpus columns
`declared_void_pointer_params_locals` and
`declared_nonvoid_data_pointer_params_locals`, grouped by `primaryCategory`.
These are independent of external GenC2Rust alignment. Version-1 sidecars can
contribute only an explicitly recorded void-pointer fact; no non-void pointer
kind is inferred from printed type strings. Pass completed external alignment
CSVs to the existing paper-facts aggregator to derive the optional cross-tool
tables without placing external-tool claims inside the analyzer output. Validate
the compact sidecars before aggregation with:

```bash
python3 scripts/validate_paper_facts.py --root test-output/multitu
```

C6g5f also records directional array-subscript facts: `array-index-read`,
`array-index-write`, and `array-index-readwrite`. A legacy `array-index` value
is retained only as a conservative parent-map ambiguity fallback and remains
active evidence without being counted as a directional dereference.

C6g5f4 extends paper facts to `schemaVersion: 3` and makes callback mediation
explicit. Each subject now has a `callbackMediation` object whose state is one
of `FormalCallbackParameter`, `UnresolvedCallback`, `FieldCallback`,
`ResolvedCallback`, `ResolvedFieldCallback`, or
`GuardedResolvedFieldCallback` (along with `None` and `Unknown`). This
prevents a caller whose callback target has already been resolved from being
reported as callback-dependent merely because its provenance includes an
indirect-call record. Report-side `semanticMediation` and stable
classification objects carry the same state plus the flags
`fieldCallbackCarrier`, `resolvedCallbackTarget`, and
`guardedCallbackSelection`.

The aggregate paper-facts output adds:

```text
callback_mediation_by_project.csv
```

and the general behavior export adds callback-mediation columns to
`parameter_behavior_rows` and field/guarded/resolved callback evidence columns
to `function_behavior_rows`:

```bash
python3 scripts/export_behavior_tables.py \
  --root test-output/multitu \
  --out-dir test-output/behavior-tables
```

## Pointer/cast prevalence sidecars (C6g4w1)

`cast-prevalence-metrics.schema.json` defines the independent, source-level
metrics sidecar emitted by:

```bash
cast-history-tool \
  --prevalence-metrics-json out/a.c.cast-prevalence.json \
  --prevalence-metrics-project-root /absolute/path/to/project \
  --prevalence-metrics-project-id project-name \
  -p build a.c
```

The project-root option is required whenever prevalence output is requested. It
makes the metric contract explicit: only declaration and cast locations whose
spelling file lies under that root are project-owned. The sidecar contains raw
source-located pointer-declaration and cast rows, and is written immediately
after AST traversal—before later cast-history/named-call fixed-point reporting.
Its `tuCounts` object is only a per-TU diagnostic; do **not** sum it across TUs
because project headers may be included repeatedly.

Generate paper-ready project totals from the raw rows instead:

```bash
python3 scripts/aggregate_cast_prevalence_metrics.py \
  --root test-output/multitu \
  --out-dir test-output/prevalence-tables
```

The aggregator deduplicates record IDs within each project and emits audit CSV/
JSONL files plus `pointer_prevalence.tex`. Pointer-typed return definitions are
excluded by default and are included only with `--prevalence-include-return-types`.

## Runtime timing sidecars (C6g5g)

`cast-runtime-timing.schema.json` describes the optional compact
`*.cast-runtime.json` sidecar emitted by:

```bash
cast-history-tool \
  --runtime-timing-dir test-output/project/runtime-metrics \
  --runtime-timing-project-id project-name \
  --runtime-timing-stage c1 \
  -p build source.c
```

The sidecar records the timing boundary from AST-consumer creation through the
end of analyzer artifact emission. Its primary table fields are:

```text
parseAndAstConstruction
castHistoryConstruction
artifactWrite
totalElapsed
```

`castHistoryConstruction` combines AST traversal, same-TU named-call/SCC
solving, cross-TU SCC solving, and imported-summary overlay application.
Those four subphases are retained separately for diagnostic CSVs, but they are
not required columns in the primary runtime table. Per-edge pointer-use timing
is intentionally not collected because it would instrument a hot transfer path
and could distort the benchmark being measured.

All standard runners publish timing sidecars only when `RUNTIME_METRICS=1`.
They aggregate final-stage sidecars into:

```text
project/runtime-duration-by-tu.csv
project/runtime-duration-by-project.csv
project/runtime-duration.tex
output-root/runtime-duration-by-tu.csv
output-root/runtime-duration-by-project.csv
output-root/runtime-duration.tex
```

Aggregate manually with:

```bash
python3 scripts/aggregate_runtime_metrics.py \
  test-output/project/runtime-metrics \
  --per-tu-csv test-output/project/runtime-duration-by-tu.csv \
  --per-project-csv test-output/project/runtime-duration-by-project.csv
```

## Void-pointer reconciliation inventory (C6g5f4d)

C6g5f4d makes the relationship between source-level `void *` prevalence and
cast-history classifications explicit without changing analysis semantics.

- Prevalence sidecars advance to `schemaVersion: 2`. Their historical
  `voidPointerDeclarations` denominator remains **direct outer `void *`
  declarations**. The sidecar now also records direct counts by declaration
  kind (`global`, `field`, `local`, `parameter`, and opt-in `return`) and
  separately records `functionTypeVoidPointerPositions` for nested `void *`
  return/parameter positions inside function-pointer types.
- Paper-facts sidecars advance to `schemaVersion: 4`. They add
  `fieldStorage`: for schema-v5 `field-storage` subjects, resolved instance
  metadata linking a storage slot such as `f::action.ActionNode` to the
  declaring aggregate field. This is Phase I instance classification only;
  it does not infer one Rust field type for all aggregate instances.

  `declaredPointerInventory`: every project-owned local/parameter direct
  pointer declaration, including declarations that never acquire an
  observation, pointer-use fact, or final cast history. The inventory is
  reporting-only; it does not alter AST flows, SCC solving, classifications,
  or existing `subjects` rows.

For complete project scope, pass the paper-facts project root alongside the
paper-facts output path. Standard GenC2Rust runners now do this automatically:

```bash
cast-history-tool \
  --paper-facts-dir test-output/project/paper-facts \
  --paper-facts-project-root /absolute/path/to/project \
  -p build source.c
```

The inventory records four mutually exclusive statuses for each declared
local/parameter pointer:

```text
classified                   # emitted subject fact with a final history
observed-no-final-history    # emitted subject fact but no final history
final-history-not-emitted    # defensive inconsistency marker
declared-no-observed-facts   # declaration never entered existing subject facts
```

Join the two sidecar families with:

```bash
python3 scripts/aggregate_void_pointer_reconciliation.py \
  --prevalence-root test-output/multitu \
  --paper-facts-root test-output/multitu \
  --out-dir test-output/void-pointer-reconciliation
```

This produces a project-level funnel and a row-level audit join:

```text
void_pointer_reconciliation_by_project.csv
void_pointer_reconciliation_rows.csv
void_pointer_reconciliation.tex
manifest.json
```

Nested function-type positions are intentionally reported separately and must
not be added to the direct-declaration prevalence total. This prevents the
historical direct-`void *` denominator from changing merely because the tool
now exposes function-pointer signature detail.

## Callback-slot relations and usage fingerprints (C6g5f4g)

C6g5f4g advances paper facts to `schemaVersion: 6` and adds two
**reporting-only** arrays. Neither array changes cast-history construction,
pointer-use equality/membership, named-call SCC scheduling, C2 import, or the
existing semantic categories.

`nestedCallbackSlotRelations` contains one row for each nested direct `void *`
position in a function-pointer type. A row identifies the callback carrier,
slot path, callback role, and a stable relation variable. When an indirect call
through that carrier is visible in the current function, the row also records
which eligible direct local/parameter `void *` subjects reached the actual
argument position. These are relational facts: a callback slot does **not**
inherit the primary category of a correlated direct subject.

`voidPointerUsageFingerprints` contains one row for each eligible direct
local/parameter `void *` declaration. It records syntactic usage flags and
reports a conservative `noFactPrimaryReason` and
`genericCandidateConfidence`. In particular,
`declared-no-observed-facts` is not automatically a polymorphism claim:
external/unresolved calls are blocked, forwarding/storing/returning is
conditional, and only locally transparent or unused values are strong generic
candidates.

Aggregate the two arrays after the normal paper-facts and reconciliation
exports. Passing the C6g5f4e/f classification table adds existing semantic
categories for correlated direct subjects; the aggregation never manufactures a
category for a callback slot or a no-fact declaration:

```bash
python3 scripts/aggregate_void_pointer_relational_facts.py \
  --root test-output/bench-genc2rust \
  --out-dir results/void-pointer-relational-facts \
  --void-pointer-classification-rows \
    results/void-pointer-reconciliation/void_pointer_classification_rows.csv \
  --strict
```

The outputs are:

```text
nested_callback_slot_relational_facts.csv
nested_callback_slot_relational_facts_by_project.csv
void_pointer_usage_fingerprint_rows.csv
void_pointer_usage_fingerprint_by_project.csv
```


## C6g5f4h / schema v7

Schema v7 adds `voidPointerUsageFingerprints[*].observedNoFinalHistory`.  It is
reporting-only and applies a stable primary pattern only when
`classificationStatus` is `observed-no-final-history`.  The object also retains
all pointer-use and syntactic-evidence flags so a primary pattern is never treated
as the only evidence for a declaration.

## C6g5f4i / schema v8: allocation origins and derived aliases

C6g5f4i advances paper facts to `schemaVersion: 8` and adds the optional,
**reporting-only** `allocationOriginDerivedAliasUses` array.  It is designed to
explain cases such as:

```c
void *raw = malloc(sizeof(struct Box));
struct Box *box = (struct Box *)raw;
box->field = 1;
free((void *)box);
```

The existing direct `void *` subject (`raw`) keeps its original cast-history
classification.  In particular, an externally declared `malloc` result may
remain `ExternalUnknown`; C6g5f4i does **not** reinterpret that category or
claim allocator ownership/lifetime semantics.  Instead, it records an
orthogonal recognized allocator-origin fact plus syntactic, **may-derived**
typed alias uses.

A root is recorded only when an eligible direct local/parameter `void *` is
initialized or assigned directly from one of the deliberately small recognized
allocator families:

```text
malloc-family: malloc, __builtin_malloc, memalign, valloc, pvalloc, aligned_alloc
calloc-family: calloc, __builtin_calloc
realloc-family: realloc, __builtin_realloc
string-duplicate-family: strdup, strndup
```

The report then follows direct local non-`void *` aliases created by initializer
or assignment from the root or another already recognized alias.  It records
may-alias derivation sites and later syntactic use flags, including field access,
dereference, pointer arithmetic, array indexing, call argument, external/indirect
call, storage, return, explicit cast, and syntactic deallocation through
`free`/`cfree`.

The mapping is intentionally path-insensitive and only records local typed
aliases. It does not perform heap/field alias analysis, prove ownership or
allocation size, recognize custom wrappers, or feed any result back into the
cast-history solver.

The existing relational-facts aggregator produces three additional artifacts:

```text
allocation_origin_derived_alias_use_rows.csv
allocation_origin_derived_alias_use_rows.jsonl
allocation_origin_derived_alias_use_by_project.csv
```

Run the same command as for C6g5f4g/h, after regenerating `PAPER_FACTS=1`
sidecars with C6g5f4i:

```bash
python3 scripts/aggregate_void_pointer_relational_facts.py \
  --root test-output/bench-genc2rust \
  --out-dir results/void-pointer-relational-facts \
  --void-pointer-classification-rows \
    results/void-pointer-reconciliation/void_pointer_classification_rows.csv \
  --strict
```

## C6g5f4j / schema v9: API-shape versus implementation eligibility

C6g5f4j advances paper facts to `schemaVersion: 9`. It adds a
**reporting-only eligibility boundary** for direct `void *` parameters and
nested `void *` callback slots. It does not change cast-history construction,
pointer-use collection or membership, SCC solving, C2/import propagation, or
existing primary semantic categories.

A direct local or parameter declaration remains in the raw declaration/API
inventory. A local is eligible for implementation-level semantic classification.
A parameter is eligible only when its owning `FunctionDecl` has a
project-owned definition with a body available in the current analyzer AST.
Prototype-only function parameters are retained with:

```text
classificationStatus: out-of-scope-unavailable-api-parameter
semanticClassificationScope: external-or-unavailable-api
ownerDefinitionAvailable: false
eligibilityExclusionReason: unavailable-owner-function-body
```

These rows are intentionally excluded from the `eligible` semantic denominator
and from generic-candidate accounting. They remain useful for API-shape
prevalence and can be exported separately as:

```text
void_pointer_api_interface_rows.csv
void_pointer_api_interface_rows.jsonl
```

The same principle applies to nested `void *` positions inside a
function-pointer parameter of a prototype-only function. For example, in an
external declaration of `bsearch`, `__compar.$0` and `__compar.$1` remain raw
API-signature slots, but are not eligible for callback-slot relational coverage
because there is no body establishing a call relation. The relational output
therefore reports both raw and eligible denominators:

```text
nested_callback_slots
eligible_nested_callback_slots
excluded_unavailable_api_nested_callback_slots
callback_argument_slots
callback_return_slots
eligible_callback_argument_slots
eligible_callback_return_slots
```

Function-pointer fields in structs and globals remain eligible relational
carriers: a project-owned call through such a field/global is still direct
source-level relation evidence. A declaration followed by a project-owned
function definition continues to use the existing definition-preferred
redeclaration policy and remains eligible exactly once.

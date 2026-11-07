# Project-level insights from cast-history output

This document lists the project-level facts that can be derived from a
cast-history runner's output, the artifact each one comes from, and the traps
that make a naive aggregation wrong. It is the rationale behind
`scripts/summarize_project_cast_profile.py`, which computes all of them.

## 1. Which artifact carries what

A runner publishes up to six families per project. Only the first is on by
default, so the breadth of the profile depends on how the run was configured.

| Artifact | `schemaName` | Enabled by | Project-level insight it supports |
|---|---|---|---|
| `*.cast-summary.json` | `cast-history-tu-link-summary` | always | translation-unit inventory, cross-TU dependency edges |
| `*.cast-report.json` | `cast-history-tu-report` | `SUMMARY_REPORTS=1` | **stable classification**: family, mediation, risk, behavior, evidence source, surface kind |
| `*.cast-prevalence.json` | `cast-history-prevalence-metrics` | `PREVALENCE_METRICS=1` | **pointer and cast distribution**: declaration kinds, `void*`/function-pointer/field split, cast kinds, type-erasure boundaries |
| `*.cast-paper-facts.json` | `cast-history-tu-paper-facts` | `PAPER_FACTS=1` | **operation characteristics**: deref/array/arithmetic/escape profile, declared outer pointer kind, primary category, coverage counters |
| `*.cast-runtime.json` | `cast-history-tu-runtime-timing` | `RUNTIME_METRICS=1` | analysis cost by phase, slowest translation unit |
| `project-summary-index.json` | `cast-history-project-summary-index` | always | call-graph size, SCC/fixpoint convergence, call-edge resolution rate |

### Directories the profile must skip

A runner publishes two kinds of directory that look like results but are not:

| Directory | Contents | Why it is excluded |
|---|---|---|
| `partial-summaries/`, `partial-reports/`, `partial-paper-facts/`, `partial-runtime-metrics/`, `partial-prevalence-metrics/` | sidecars of translation units that **failed during C1** | the runner states these are never fed to the index; they describe an aborted analysis, so counting them mixes incomplete data into every distribution |
| `.work/` (kept by `--keep-intermediates`) | a full `c1-`, `imp01-`, `local-overlay-` copy of every sidecar | counting them multiplies every figure by the number of passes |

`partial-status/<stem>.cast-analysis-status.json` is read, but only to report
*which* translation units failed and why. On a 2-TU project with one failing
TU, including the `partial-*` sidecars inflated the declaration count from 6
to 7 and the cast count from 9 to 12.

A profile built from link summaries alone is nearly empty: the compact link
summary deliberately omits rendered histories, metrics and classification. To
get a complete overview the run needs at minimum:

```bash
PREVALENCE_METRICS=1 PREVALENCE_INCLUDE_RETURN_TYPES=1 PAPER_FACTS=1 SUMMARY_REPORTS=1 RUNTIME_METRICS=1
```

## 2. The insight catalogue

### Pointer population
- Pointer declarations by kind (parameter / local / field / global / return).
- `void*` share overall and by declaration kind — the size of the type-erased
  surface the project exposes.
- Function-pointer declarations and `void*` positions nested inside function
  types: the callback surface, which is what makes mediation analysis necessary.
- Most-declared canonical pointer types: the project's dominant data structures.
- Declaration sites per file: where pointer-heavy code lives.

### Cast distribution
- Explicit vs implicit split. Implicit casts dominate by an order of magnitude
  in ordinary C, so an "N casts" figure is meaningless without the split.
- Cast kinds, and the pointer→pointer subset.
- Type-erasure boundaries: to-`void*`, from-`void*`, byte-pointer. These three
  counters, not the raw cast total, size the migration problem.
- Most frequent explicit cast pairs. In practice the top pointer pairs name the
  project's opaque-handle downcasts.

### Classification
- Semantic family, mediation kind, risk tier, behavior, history shape,
  surface kind — the distributions the paper tables are built from.
- Evidence source: how much of the classification rests on project-wide SCC
  solving versus purely local evidence. This is a confidence measure.
- **Surface-wild refinement**: of the subjects that look wild at the surface,
  how many resolve to a structured family (direct/field/callback-mediated
  concrete specialization, byte view, opaque transport) rather than staying
  unknown. This is the single most load-bearing project-level number.
- Project-owned versus system-header subjects. Subjects from `/usr/include`
  (for example `bsearch::__base`) are classified too and must be separated
  before any claim about *the project*.

### Operation characteristics
- Transport-only versus actively dereferenced subjects.
- Dereference reads, dereference writes, array indexing and its direction,
  pointer arithmetic, external escape.
- Declared outer pointer kind, and the share of subjects that reached a final
  history at all — the analysis's own coverage measure.

### Topology and solver health
- Functions, dependency edges, parameter transfer edges, alias-returning
  functions, recursive SCC functions.
- Fixpoint convergence and whether a solver stopped on its time limit.
- Call-edge resolution rate, with the unresolved endpoints named.

### Cost
- Parse/AST, cast-history construction and artifact write time, per phase and
  summed; slowest translation unit.

## 3. Aggregation traps

These are the reasons a project-level figure cannot be a sum of per-TU counters.

1. **Header declarations are counted once per including TU.** Summing
   `tuCounts` across zlib's 15 translation units reports 2384 pointer
   declarations; there are 510 distinct declaration sites — a 4.7x inflation,
   almost all of it `zlib.h`. Every prevalence row carries a source-located
   `id`, so project-level counts must deduplicate on it. The same applies to
   classification rows (deduplicate on subject plus definition location) and to
   paper-facts subjects (deduplicate on `subjectId`).
2. **The record `id` alone is not a safe dedup key.** It is
   `<kind>:<file>:<line>:<col>:<discriminator>`, where `<file>` is the path
   exactly as clang reported it. That path is neither normalized
   (`/p/src/../src/a.c` and `/p/src/a.c` are two keys for one declaration) nor
   unique across projects: two projects analyzed under one output root can
   carry the same relative path, so their rows collide and their statistics
   are conflated. Key on **the project plus the resolved, normalized source
   location**, keeping the discriminator the analyzer encoded —
   `declarationKind` for a declaration, `castKind` plus implicit/explicit for
   a cast. Dropping that discriminator silently merges an explicit
   `CStyleCastExpr` with the `ImplicitCastExpr` at the same position; on zlib
   that lost 24 of 11540 casts.
3. **Per-round scratch directories duplicate everything again.** Import rounds
   leave copies under `.tmp-intermediates/`, `c1-*` and `import-round-*`. A
   recursive glob that does not exclude them multiplies counts by the round
   count.
4. **The same source location can yield conflicting rows across TUs.** A
   declaration or cast inside a macro or a conditionally-compiled header may be
   recorded with different types in different translation units while keeping
   the same `file:line:column` id. zlib produces 25 such conflicting cast ids.
   Deduplication must therefore pick a representative deterministically (this
   profiler keeps the first in sorted-path order, matching
   `aggregate_cast_prevalence_metrics.py`, which warns about them).
5. **`paperWildness` and `stableClassification` are different vocabularies.**
   The paper-facts sidecar carries the pre-refinement family
   (`ConcreteSpecialization`, risk `High`); the report carries the refined one
   (`DataFieldMediatedConcreteSpecialization`, risk `DangerousReinterpretation`).
   They are not interchangeable, and mixing them silently double-counts.
6. **System-header subjects are in the data.** Filter on the project root
   before reporting project characteristics.
7. **Report sidecars are large.** zlib's are 228 MB against 12 MB of
   paper-facts. Identify artifacts by reading the first few KB rather than
   parsing, and allow the classification pass to be skipped.

## 4. Known defects this surfaces

Found while profiling zlib 1.2.11 with the current `cast-history-dev` build.

### A relative source path in `compile_commands.json` silently empties the prevalence sidecar

`PrevalenceMetrics::projectLocation` keeps a row only when
`sourceManager.getFilename()` starts with `--prevalence-metrics-project-root`.
Clang reports the filename exactly as the compilation database spells it, so a
Bear-style entry whose `arguments` end in a relative path

```json
{"directory": "/p", "arguments": ["clang", "-c", "-I.", "-o", "/dev/null", "good.c"],
 "file": "/p/good.c"}
```

yields the filename `good.c`, which never matches the absolute project root.
The runner's own safeguards do not help: it passes the **absolute** source path
on the command line and an absolute `--prevalence-metrics-project-root`, but
the filename clang reports comes from the database entry, not from the path the
tool was invoked with.

**This cannot be repaired after the fact.** The records are never constructed,
so the sidecar is written with empty arrays:

```
project.root        : /p
pointerDeclarations : []
casts               : []
tuCounts nonzero    : {} (ALL ZERO)
```

There is no path in the artifact to normalize and no row to re-key — the data
does not exist. Path normalization and project-scoped keys in an aggregator are
worth having for other reasons (trap 2 above), but they cannot recover this.

The run looks clean from the outside: the runner reports `prevalence sidecars: 1`
(it counts files, not content), every project is `OK`, and it exits 0. The
profiler reports it as a critical finding and exits non-zero instead.

The same project-root prefix test scopes the paper-facts declared-pointer
inventory, so `--paper-facts-project-root` is emptied by the same cause:
`declaredPointerInventory` is `[]` and `coverage.declaredPointerInventoryRows`
is 0 while `coverage.subjectRows` is unaffected.

Three ways to fix it, in order of preference:

1. **In the analyzer.** Make the reported filename absolute before the prefix
   test (resolve a relative path against the compile command's working
   directory) and lexically normalize both it and `projectRoot`. One change,
   every consumer fixed.
2. **In the runner.** `discover_sources` already resolves every source to an
   absolute path. It could write a normalized copy of `compile_commands.json`
   into `$OUT/` with absolute source arguments and point `-p` at that. No C++
   change, and it also fixes paper-facts.
3. **Detect and refuse.** Treat an all-zero prevalence sidecar beside a
   non-empty translation unit as an error rather than a result. This is what
   the profiler does today, but it only catches the problem after a full run.

### `tu.sourceFile` in the prevalence sidecar is a dangling reference

`CastHistoryVisitor::emitPrevalenceMetricsJson` passes
`mainFileNameForSummary()` — which returns `std::string` by value — into a
`llvm::StringRef` parameter that is then stored in a `llvm::json::Value`.
`json::Value`'s `StringRef` constructor is non-owning, so the temporary dies
before the document is serialized and the field is written from freed memory:

```
"sourceFile": "\u0000-\u001eoÿ\u007f\u0000..."
```

The file is still structurally valid JSON but is no longer valid UTF-8, so any
consumer that decodes it strictly — `json.loads(path.read_text(encoding="utf-8"))`,
the pattern `scripts/extract_stable_classifications.py` uses — raises
`UnicodeDecodeError` on it. All 15 of zlib's prevalence sidecars are affected;
the report, paper-facts, link-summary and runtime sidecars are clean.
Three separate repo scripts already carry `errors="replace"` at their read
sites — `aggregate_cast_prevalence_metrics.py`,
`aggregate_void_pointer_reconciliation.py` and `validate_json_summaries.py`
(which globs every `*.json`, so it reaches the prevalence sidecars). Nothing
crashes today, but nothing is fixed either: the corruption is read in and
passed on, and `tu.sourceFile` cannot be used to attribute a sidecar to its
translation unit.

The corruption has a clear signature — the first 16 bytes of the freed heap
buffer are reused and the tail survives:

```
'\x9dB\xd9=\xc5U\x00\x00\xc7\xa9\x0b\xc9\xd3&\x12\xe9ome-user-chp/.../zlib/adler32.c'
```

`tests/prevalence_sidecar_tu_source_file_contract.py` pins this down: it
performs the strict read a naive consumer would, then checks that
`tu.sourceFile` actually names a source file. All 15 of zlib's sidecars fail
it.

```bash
python3 tests/prevalence_sidecar_tu_source_file_contract.py <output-root>/zlib
``` The fix is to store an owning string
(`std::string`, so `json::Value` copies it) at that call site instead of
binding the temporary to a non-owning `StringRef`.

### Classification vocabulary has drifted from the schema

`stableClassification.family` is `SemanticMediationSummary::semanticFamily` and
`stableClassification.risk` is `historyRiskClassName(HistoryRiskClass)`. Both
produce labels that `schemas/stable-classification.schema.json` does not list
in `x-c6f-known-values`, so `validate_classification_labels.py
--strict-vocabulary` cannot be used as a gate.

**Missing from the schema's `family` list (8).** All eight are reachable:
the first five are paper families that pass through the mediation layer
unchanged, the three `Guarded*` ones come from the `BranchTaggedUnionLike`
branch.

| label | origin |
|---|---|
| `ByteMediatedReinterpretation` | paper family, passes through unchanged |
| `MultiTargetReinterpretation` | paper family, passes through unchanged |
| `MultiTypeAccess` | paper family, passes through unchanged |
| `DangerousProvenance` | paper family, passes through unchanged |
| `Unknown` | paper family default |
| `GuardedDataFieldMediatedSpecialization` | `BranchTaggedUnionLike` + data field |
| `GuardedNestedDataFieldMediatedSpecialization` | `BranchTaggedUnionLike` + nested data field |
| `GuardedDataFieldCallbackMediatedSpecialization` | `BranchTaggedUnionLike` + data field + callback |

On zlib these account for 121 of 273 classified subjects (44%).

**Missing from the schema's `risk` list (8).** `HistoryRiskClass` has twelve
enumerators; the schema lists four of them plus `High`.

`ByteViewRead`, `ByteViewWrite`, `BranchPolymorphicWrite`,
`SuspiciousReinterpretation`, `DangerousReinterpretation`, `DangerousProvenance`,
`HigherOrderUnresolved`, `ExternalSink`

**Listed but never emitted.** Worth deciding on rather than leaving:
family `ConcreteSpecialization`, `MixedMediatedConcreteSpecialization` and
`UnknownMediation` (the first is always rewritten by the mediation layer);
risk `High` — it is produced by `paperSemanticRiskTier` for
`paperWildness.semanticRiskTier`, but never by `historyRiskClassName`, so it
cannot appear in `stableClassification.risk`.

**`validate_classification_labels.py` is less stale than the schema.** Its
`ALLOWED_SEMANTIC_FAMILIES` is missing only `DangerousProvenance` and
`MultiTypeAccess`; its `ALLOWED_RISK_TIERS` is missing the same eight risk
labels as the schema.

## 5. Using the script

```bash
# one project
python3 scripts/summarize_project_cast_profile.py \
    --root test-output/real-projects/zlib-1.2.11

# a whole corpus; adds corpus-overview.csv and a per-project profile each
python3 scripts/summarize_project_cast_profile.py \
    --root test-output/real-projects \
    --out-dir results/corpus-profile
```

Each project directory gets `cast-profile.md`, `cast-profile.json`,
`cast-profile.html` (a self-contained dashboard, light and dark) and
`tables/*.csv` ready for the paper tables. A terminal quick-glance panel is
printed unless `--quiet` is passed.

Useful flags: `--no-classification` skips the large report sidecars,
`--max-file-mb` caps sidecar size, `--mode project|corpus` overrides the
auto-detection, `--project-root` names the source tree when the prevalence
sidecars do not record it, `--include-partial` counts the sidecars of
translation units that failed in C1, `--include-intermediates` reads `.work/`,
and `--strict` makes a serious finding fail the run.

A root holding exactly one project is reported as that project, so pointing at
either `<output>/` or `<output>/<project>/` of a single-project run gives the
same report; pass `--mode corpus` to force the comparison table.

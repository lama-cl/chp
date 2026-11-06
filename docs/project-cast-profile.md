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
2. **Per-round scratch directories duplicate everything again.** Import rounds
   leave copies under `.tmp-intermediates/`, `c1-*` and `import-round-*`. A
   recursive glob that does not exclude them multiplies counts by the round
   count.
3. **The same source location can yield conflicting rows across TUs.** A
   declaration or cast inside a macro or a conditionally-compiled header may be
   recorded with different types in different translation units while keeping
   the same `file:line:column` id. zlib produces 25 such conflicting cast ids.
   Deduplication must therefore pick a representative deterministically (this
   profiler keeps the first in sorted-path order, matching
   `aggregate_cast_prevalence_metrics.py`, which warns about them).
4. **`paperWildness` and `stableClassification` are different vocabularies.**
   The paper-facts sidecar carries the pre-refinement family
   (`ConcreteSpecialization`, risk `High`); the report carries the refined one
   (`DataFieldMediatedConcreteSpecialization`, risk `DangerousReinterpretation`).
   They are not interchangeable, and mixing them silently double-counts.
5. **System-header subjects are in the data.** Filter on the project root
   before reporting project characteristics.
6. **Report sidecars are large.** zlib's are 228 MB against 12 MB of
   paper-facts. Identify artifacts by reading the first few KB rather than
   parsing, and allow the classification pass to be skipped.

## 4. Known defects this surfaces

Found while profiling zlib 1.2.11 with the current `cast-history-dev` build.

### A relative source path in `compile_commands.json` silently empties the prevalence sidecar

`PrevalenceMetrics::projectLocation` keeps a row only when
`sourceManager.getFilename()` starts with `--prevalence-metrics-project-root`.
Clang reports the filename exactly as it appears in the compilation database,
so a relative entry such as

```json
{"directory": "/src/zlib", "arguments": ["clang", "-c", "adler32.c"], "file": "/src/zlib/adler32.c"}
```

yields the filename `adler32.c`, which never matches the absolute project root.
Every declaration and cast is dropped and the sidecar is written with all
counters at zero and no diagnostic. The same compilation database produces a
complete inventory once the source argument is absolute. Bear-generated
databases frequently use relative arguments, so this is likely to hit real
pilot runs. The profiler reports it as a critical finding rather than printing
a page of zeros.

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
`scripts/aggregate_cast_prevalence_metrics.py` already carries
`errors='replace'` at its read site, which looks like an undocumented
workaround for exactly this. The fix is to store an owning string
(`std::string`, so `json::Value` copies it) at that call site instead of
binding the temporary to a non-owning `StringRef`.

### Classification vocabulary has drifted from the schema

The analyzer emits families (`ByteMediatedReinterpretation`,
`MultiTypeAccess`, `MultiTargetReinterpretation`,
`GuardedDataFieldMediatedSpecialization`, `DangerousProvenance`, `Unknown`) and
risk tiers (`DangerousReinterpretation`, `DangerousProvenance`) that are not in
`x-c6f-known-values` in `schemas/stable-classification.schema.json`. Those
labels account for 121 of zlib's 273 classified subjects (44%), so
`validate_classification_labels.py --strict-vocabulary` cannot currently be
used as a gate until the schema is refreshed.

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
sidecars do not record it, and `--strict` makes a serious finding fail the run.

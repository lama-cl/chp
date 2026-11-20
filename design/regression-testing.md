# Regression Testing — Design & Plan

Status: **Phases 0–2 done (§8–§10); waiting for go-ahead on Phase 3** (2026-11-20)
Owner: project maintainers · Implementers: humans or agents (follow the task list in §5)

---

## 1. TL;DR

Today nothing compares analyzer output against an expected result automatically.
This plan adds:

1. A small, fixed **regression set**: `examples/demo*.c` + 5 projects from `test_projects/projects/`.
2. **Full normalized expected outputs** committed next to each fixture, in an `expected/` folder, with `.expected` in the file name.
3. One driver, `scripts/regress.py` (`run` / `update`), that runs the tool, normalizes, and diffs.
4. `ctest` wiring for the regression diff, the existing `tests/` checkers, and the summary validators.
5. A **perf check** based on the existing `*.cast-runtime.json` timing sidecars (warn-only).

Build requirement: **LLVM 19 or newer** (latest preferred). Expected outputs must be generated with LLVM ≥ 19.

---

## 2. Current state (findings)

| Area | What exists | Where the result is checked |
|---|---|---|
| `examples/*.c` (86 single-TU programs) | `scripts/run_all_examples_normalized.sh` runs `cast-history-tool --debug-dump`, writes `examples-output/<name>-output.txt`, and normalizes absolute paths | **Only the exit code.** No expected outputs are committed; diffs are done by hand. |
| `test_projects/projects/` (132 multi-TU projects) | `scripts/cast_history_project.sh` (+ older `run_all_multitu_projects*.sh`) produce `.txt` reports, `*.cast-summary.json`, `project-summary-index.json`; `--normalize-paths` exists "for diffing" | **No automatic diff.** 129 `expected/README.expected.txt` files describe intended behaviour in prose; checked by a human. |
| `tests/` | `verify_*_report.py --fixture pNNN <report>` (substring checks), `*_source_contract.py` (static checks on headers and scripts), `*_smoke.py/.cpp` | Real assertions, but **not wired up**. Nothing runs them automatically. |
| `scripts/validate_*.py` | Schema, text/JSON consistency, label and paper-fact validators | Structural only; not wired up. |
| `p91_*/expected/verify_p91*.py` | Executable per-project checkers | Run manually. |
| `*-output-benchmarks/*.zip` | GenC2Rust comparison runs | Cross-tool comparison (`join-*.py`), **not** regression expected outputs. |
| CMake | `add_executable(cast-history-tool …)` only | No `enable_testing` / `add_test`. `CLAUDE.md` mentions `ctest`, `lit` and FileCheck, but none of them are set up. |

The current workflow is to run the drivers on the base commit and on the change, `diff -r` the two output directories, and read the `README.expected.txt` files.

---

## 3. Decisions (agreed)

| # | Decision |
|---|---|
| D1 | **LLVM ≥ 19 (latest preferred)** for building and for generating expected outputs. If only an older LLVM is available, stop; do not generate expected outputs with it. |
| D2 | **Small regression set**, not the full suite (see §4.1). The full suite stays a manual or optional run. |
| D3 | **Full output** expected files (not an excerpt), so they can be piped or dumped for viewing. |
| D4 | Expected files live **next to the fixture** in an `expected/` folder and contain `.expected` in the name, so `find -name '*.expected*'` finds them all. |
| D5 | **Runtime is kept** and used for a perf-regression check (warn-only, tolerance-based), not for an exact diff. |
| D6 | Phase 4 (corpus/bench runs, CI workflow, CLAUDE.md rewrite) is **deferred** and needs its own plan. |

---

## 4. Design

### 4.1 Regression set

**Examples**: `examples/demo1.c` … `examples/demo6.c`

**Projects** (`test_projects/projects/`):

| Project | Why |
|---|---|
| `p01_direct_cross_tu` | Basic acyclic cross-TU direct call (C1 → C2 index → C3 import) |
| `p02_cross_tu_return` | Cross-TU return-value flow |
| `p03_cross_tu_mutual_recursion` | Cross-TU recursion / SCC |
| `p91_deref_fnptr_higher_order_callee_normalization` | Function pointers and higher-order calls; has `expected/verify_p91*.py` |
| `p126_recursive_parallel_observation_dedup` | Covered by most `tests/verify_c6g5{p,q,r,s,t,u}_*` checkers |

The set is defined in one list (`scripts/regress.py` or a sibling `regress-set.txt`) so it is easy to extend.

### 4.2 Expected-file layout

```
examples/
  demo1.c
  expected/
    demo1.expected.txt                      # normalized --debug-dump output
    demo1.runtime.expected.json             # timing baseline (perf check only)
    meta.info                               # LLVM version the files came from (not compared)

test_projects/projects/<project>/
  a.c  b.c
  expected/
    README.expected.txt                     # existing prose intent (unchanged)
    verify_*.py                             # existing checkers (unchanged)
    a.c.expected.txt                        # normalized per-TU text report
    summaries/a.c.cast-summary.expected.json             # normalized, key-sorted
    prevalence-metrics/a.c.cast-prevalence.expected.json # normalized, key-sorted
    project-summary-index.expected.json     # normalized, key-sorted index
    runtime.expected.json                   # timing baseline (perf check only)
    meta.info                               # LLVM version the files came from (not compared)
```

Rule: the driver's output path, with `.expected` inserted before the last extension (`a.c.txt` → `a.c.expected.txt`). Subfolders mirror the driver output, so an expected file maps back to its source with no lookup table. The `.expected` part sits just before the real extension, so editors keep syntax highlighting.

`regress.py` owns every `*.expected.txt|json` in these folders **except** `README.expected.txt` and `runtime.expected.json`. It never touches other files (`verify_*.py`, `README.expected.txt`).

Find everything: `find examples test_projects/projects -path '*/expected/*' -name '*.expected*'`

### 4.3 Normalization (applied to both actual and expected output)

1. Absolute paths: tool (repo), project and output roots become `<tool-root>`, `<project-root>`, `<output-root>` (longest path replaced first). Reuse the logic in `run_all_examples_normalized.sh` and the `--normalize-paths` option of `cast_history_project.sh`.
2. JSON is re-serialized with sorted keys, 2-space indent and a trailing newline.
3. Fields that change from run to run (found in Phase 0) are removed or replaced with a placeholder. The removed fields are listed explicitly in `regress.py`, not hidden behind regexes.
4. Timing data is **removed from the diffed files** and kept only in the `runtime.expected.json` files (see §4.5).

### 4.4 `scripts/regress.py`

```
regress.py run    [--only NAME]...                 # diff; exit 1 on any mismatch
regress.py update [--only NAME]...                 # rewrite expected/*.expected* (not baselines)
regress.py perf   [--only NAME]... [--runs 3] [--tolerance 0.25] [--min-delta-ms 50] [--strict]
regress.py perf --update                           # rewrite runtime baselines
# common: --tool PATH  --clang CMD  --out-dir DIR (default build/regress-out)  --tu-timeout SEC
# NAME: a fixture (demo1, p01_direct_cross_tu), or "examples" / "projects"
# exit: 0 ok, 1 mismatch / fixture failure / strict slowdown, 2 usage or environment problem
```

- Calls the existing drivers (`run_all_examples_normalized.sh`, `cast_history_project.sh`) and does not re-implement them.
- Writes output to `build/regress-out/` (cleaned per fixture) and never into the source tree, except for `update` / `perf --update` and the gitignored `compile_commands.json`. A normalized copy of every compared file goes to `build/regress-out/normalized/<repo path>` for inspection.
- Reports expected files that are missing and expected files that are no longer produced (`STALE`). `update` deletes stale ones.
- Refuses to `update` if any fixture fails.
- Prints a unified diff for each mismatched file and ends with a `PASS n / FAIL m` summary.
- Reads the LLVM version from `cast-history-tool --version` and refuses `update` / `perf --update` if it is below 19. Picks `clang-<major>` for builtin headers. `run` warns when `expected/meta.info` names a different LLVM major version.
- Builds `compile_commands.json` for each project before running (as `CLAUDE.md` requires), using `scripts/regenerate_compile_commands.py` or the driver.
- Uses only Python standard-library modules (`json`, `difflib`, `subprocess`, `argparse`).

### 4.5 Perf check

- Metrics: examples → `wallMs`; projects → `wallMs` (whole driver run), `indexBuildMs` (final index `buildMetadata.timingsMilliseconds.totalBeforeWrite`, see §8) and `tuTotalElapsedMs` (sum of the per-TU `*.cast-runtime.json` `totalElapsed`).
- Baseline: `runtime.expected.json` per fixture, median of `--runs` (default 3), written only by `perf --update`, so ordinary `update`s don't churn timings.
- A slowdown also has to be at least `--min-delta-ms` (default 50 ms), so tiny numbers don't flap.
- Check: flag a fixture if its time exceeds `baseline × (1 + tolerance)`, with the tolerance set to 25% by default. **Warn-only** (exit 0) unless `--strict` is given, because timings depend on the machine.
- Baselines should be refreshed together with the other expected outputs and come from the same machine class.

### 4.6 `ctest` wiring

`CMakeLists.txt` gets `enable_testing()` and these tests:

| ctest name | Label | Command |
|---|---|---|
| `regress-examples` | `regress` | `regress.py run --only examples` |
| `regress-projects` | `regress` | `regress.py run --only projects` |
| `perf` | `perf` | `regress.py perf` (warn-only) |
| `contract-<name>` | `contracts` | each `tests/*_source_contract.py` (no tool run, finishes in seconds) |
| `verify-<checker>-<fixture>` | `checkers` | `tests/verify_*.py --fixture pNNN <report>`, but only for fixtures in the regression set (p126) |
| `verify-p91*` | `checkers` | `test_projects/projects/p91_*/expected/verify_p91*.py` |
| `smoke-<name>` | `checkers` | `tests/*_smoke.py` / `.cpp` that run standalone |
| `validate-summaries` | `checkers` | `scripts/validate_json_summaries.py`, `validate_cast_summary_schema.py` on the produced summaries |

`checkers` tests that need a report depend on a ctest `FIXTURES_SETUP` step that runs the tool once and leaves its output in `build/regress-out/`.

---

## 5. Task list (actionable; do in order)

Each task ends with its **done when** condition. Do not start a phase until the previous phase is done.

### Phase 0: build and stability check (no source changes)
- [x] **T0.1** Install or confirm LLVM ≥ 19 (latest). Configure with `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=$(llvm-config --cmakedir) -G Ninja`, then build with `ninja -C build`.
  *Done when* `build/cast-history-tool` exists and `llvm-config --version` ≥ 19.
- [x] **T0.2** Run the regression set (§4.1) **twice** with path normalization and `diff -r` the two runs.
  *Done when* every field that changes between runs is listed (file, field, example).
- [x] **T0.3** Report the T0.2 findings to the maintainer before continuing.

### Phase 1: driver
- [x] **T1.1** Add `scripts/regress.py` with `run`, `update` and `perf` as specified in §4.3–4.5.
- [x] **T1.2** Encode the regression-set list and the list of removed fields.
  *Done when* `regress.py run` on a fresh build reports a clear "missing expected file" for every fixture (no crash).

### Phase 2: expected outputs
- [x] **T2.1** `regress.py update` (LLVM ≥ 19) creates `expected/*.expected*` files following §4.2.
- [x] **T2.2** Commit the expected files **in a separate commit** from the code changes.
  *Done when* `regress.py run` passes twice in a row and `regress.py perf` runs without error.
- [x] **T2.3** Sanity check: make a deliberate change to one demo, confirm `run` fails with a readable diff, then revert.

### Phase 3: ctest
- [ ] **T3.1** Add `enable_testing()` and the tests from §4.6.
- [ ] **T3.2** List any `tests/*` script that cannot run standalone, with the reason.
  *Done when* `cd build && ctest --output-on-failure` passes and `ctest -L contracts` finishes in seconds.

### Phase 4: deferred (needs its own plan)
- Corpus/bench runs (`test_projects/bench-*`), CI workflow, rewriting the lit/FileCheck section of `CLAUDE.md`, growing the regression set.

---

## 6. Workflow after implementation

```bash
# verify a change
cd build && ctest --output-on-failure          # or: ctest -L regress
# behaviour changed on purpose -> refresh expected outputs and review the diff
python3 scripts/regress.py update && git diff -- '*.expected*'
# view an expected output
cat examples/expected/demo1.expected.txt | less
```

Rule: a PR that changes `*.expected*` files must explain **why** the output changed. Expected outputs record current behaviour; whether that behaviour is correct is still judged against the `README.expected.txt` files and the `tests/verify_*` checkers.

## 7. Risks

| Risk | Mitigation |
|---|---|
| Output depends on the LLVM version | D1; `update` refuses LLVM < 19; record the LLVM version in each expected file's header or in a single `expected/LLVM_VERSION` file |
| Output changes between runs (hash ordering, temp paths) | Phase 0 finds these fields; fix them at the source if cheap, otherwise remove them in normalization |
| Timings are noisy | Median of 3 runs, 25% tolerance, warn-only by default |
| Expected files get updated without review | Separate commit for expected files; PR rule in §6 |

---

## 8. Phase 0 results (2026-11-20)

### Environment
- Ubuntu 24.04. apt.llvm.org is blocked by the network policy. The newest LLVM in the Ubuntu archive is **20.1.2** (`llvm-20-dev libclang-20-dev clang-20`).
- Configure needs `PATH=/usr/lib/llvm-20/bin:$PATH` and an explicit `-DClang_DIR=/usr/lib/llvm-20/lib/cmake/clang`. The older system `clang`/`llvm-config` (18) is still on PATH and gets picked otherwise:
  ```bash
  export PATH=/usr/lib/llvm-20/bin:$PATH
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=$(llvm-config --cmakedir) \
        -DClang_DIR=/usr/lib/llvm-20/lib/cmake/clang \
        -DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20 -G Ninja
  ninja -C build
  ```
  Build time is about 25 s on 4 cores (1305 warnings, 0 errors).
- `compile_commands.json`: `regenerate_compile_commands.py --cc clang-20 --project-sets projects` (gitignored output). **Limitation:** `--project-glob projects/<one-project>` treats a project that has no `compile_commands.json` yet as a *group of projects* and writes nothing. `regress.py` should generate the selected projects itself, or the script needs a fix.
- Drivers called with `--clang clang-20` (so the builtin headers match the tool) and `--normalize-paths`.

### Results
All 6 demos and all 5 projects ran with exit code 0 in both runs. Total ≈ 15 s per run, of which p126 takes about 12 s.

| Output | Same in both runs? | What differs | Plan |
|---|---|---|---|
| `examples/*-output.txt` | yes | — | diff exactly. Note: the `Command:` line includes the tool path *as passed*; `regress.py` must pass a relative `./cast-history-tool` or normalize it |
| `<proj>/<tu>.txt` reports | yes | — | diff exactly |
| `summaries/*.cast-summary.json` | yes | — | diff exactly (key-sorted) |
| `prevalence-metrics/*.cast-prevalence.json` | yes* | `project.root` is an absolute **source** path (not normalized) | diff; normalize `root` |
| `project-summary-index.json` | **no** | `summaryRoot` (absolute output path); `buildMetadata.inputSummaryFingerprint` (hashes absolute file paths, `build_summary_index.py:120`); `buildMetadata.timingsMilliseconds.*` | diff after normalizing `summaryRoot`, removing the fingerprint, and moving timings to the perf baseline |
| `runtime-metrics/*.cast-runtime.json`, `runtime-duration*.csv/.tex` | no | timings only | perf baseline only, not diffed |
| `.cast-history-output` marker, `commands.log`, `run.log`, `projects-summary.tsv` | no / not needed | timestamps, elapsed time | not diffed |

\*Same in both runs here only because the sources were read from the same location both times; it will differ on another machine.

No ordering changes (e.g. from hash maps) were seen in any diffed file.

### Perf finding (affects §4.5)
In p126 the analyzer itself takes **~37–70 ms** per TU; the rest of the 12 s is the **index build** (`build_summary_index.py`, `timingsMilliseconds.solvedOther` ≈ 8.3 s, run twice: c1-raw and final). The per-TU `*.cast-runtime.json` sidecars do not capture this. **Change to §4.5:** the perf baseline must also record `project-summary-index.json → buildMetadata.timingsMilliseconds` and the project's total wall time, not just the per-TU sidecars. Run-to-run variation seen: index ≈ ±1%, per-TU ms values ±10–30% (small numbers), so compare the per-TU sidecars on totals only.

---

## 9. Phase 1 results (2026-11-20)

Branch: `claude/regression-test-harness`.

- Added `scripts/regress.py` (stdlib only) as specified in §4.2–4.5.
- Fixed `scripts/regenerate_compile_commands.py`: `--project-glob <project>` now treats a directory that holds sources as one project, even before it has a `compile_commands.json` (the limitation in §8). `regress.py` uses it to create missing compile databases.
- **Done-when check:** on a fresh build with no expected files, `regress.py run` reports `MISSING expected file …` for all 11 fixtures (35 files) and exits 1.
- **Rehearsal of Phase 2** (all generated files deleted afterwards; nothing committed):
  - `update` wrote 35 files; `run` then passed 11/11 twice in a row.
  - A cast added to `demo1.c` produced a readable unified diff and exit 1.
  - A fake extra expected file was reported as `STALE`.
  - `perf` without baselines prints `NOBASE` and exits 0; after `perf --update`, a baseline edited to be 7× too fast is flagged `SLOWER` and `--strict` exits 1.
  - Unknown `--only` names and a missing tool exit 2.
- Runtime of the full set: ≈ 15 s (p126 ≈ 12 s, almost all in the index build).

---

## 10. Phase 2 results (2026-11-20)

- `regress.py update` (LLVM 20.1.2) wrote 35 expected files plus a `meta.info` per `expected/` folder. They are committed on their own ("Add expected outputs for the regression set"): 41 files, ≈ 6.1 MB, 168k lines. Most of that is p126, whose summary (2.2 MB) and index (3.3 MB) are large.
- `regress.py run` passed 11/11 twice in a row (exit 0).
- T2.3: a cast added to `demo3.c` gave a 63-line diff and exit 1; reverted.
- `regress.py perf --update` wrote 11 `runtime.expected.json` baselines (median of 3) in a separate commit. **They come from a 4-CPU x86_64 cloud container**; refresh them on the machine where perf is checked (`regress.py perf --update`) before trusting `SLOW` results.

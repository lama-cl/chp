# cast_history_project.sh

Builds the cast history of one or many C/C++ projects (in-tree or out-of-tree)
into an output directory. Replaces `run_all_multitu_projects-rp.sh` for
benchmarks that live outside this repository.

Run `scripts/cast_history_project.sh --help` for every option.

## What you need

| Need | How it is found | Override |
|---|---|---|
| `cast-history-tool` binary | `./cast-history-tool`, `./build/`, `<repo>/build/` | `--tool PATH` or `TOOL=` |
| `compile_commands.json` | `PROJECT/`, `PROJECT/build/`, or each subdirectory of a root | `--compdb PATH`, or `--synthesize-compdb` |
| clang matching the tool's LLVM | `clang-<tool LLVM major>`, then `clang` | `--clang CMD`, `--resource-dir DIR` |
| `python3`, `build_summary_index.py`, `aggregate_runtime_metrics.py` | next to the script | `--python`, `--build-index`, `RUNTIME_AGGREGATOR=` |

Build the tool once (any LLVM/Clang with dev packages, e.g. 20):

```bash
cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-20/lib/cmake/llvm -DClang_DIR=/usr/lib/llvm-20/lib/cmake/clang
cmake --build build -j
```

## Sample invocations

One project that already has `build/compile_commands.json`:

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool -o /data/out /data/benchmarks/lua-5.4.1
```

A directory holding several projects (every subdirectory with a compile
database is one project; results go to `/data/out/<project>/`):

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool -o /data/out /data/benchmarks
```

From inside a project, skipping tests and vendored code, two import rounds:

```bash
cd ~/src/zlib
/path/to/chp/scripts/cast_history_project.sh --tool /path/to/chp/build/cast-history-tool \
    --exclude '/(test|contrib|third_party)/' --rounds 2 -o ~/out/zlib
```

Projects without any build metadata (generates a compile database from a
source scan; flags are guesses, add `-I`/`-D` with `--cflags`):

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool --synthesize-compdb \
    --cflags "-Iinclude -DNDEBUG" -o /data/out /data/benchmarks/some-project
```

A corpus of unrelated single-file programs (no index, no import rounds):

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool --synthesize-compdb --single-tu \
    -o /data/out /data/bench-genc2rust
```

Skip the optional stages for a quick run (runtime timings, prevalence
sidecars, partial sidecars of failed TUs):

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool --minimal -o /tmp/quick ./my-project
```

Extra sidecars and diff-friendly reports:

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool --reports --paper-facts \
    --normalize-paths -o /data/out /data/benchmarks/lua-5.4.1
```

A compile database elsewhere, with a specific clang:

```bash
scripts/cast_history_project.sh --tool build/cast-history-tool --clang clang-20 \
    --compdb /data/build-dir -o /data/out /data/src/project
```

## Output

```
OUT/run.log                    everything printed in the run
OUT/projects-summary.tsv       one row per project: status, TUs, summaries, failed, stale
OUT/runtime-duration-*.csv/.tex  timing tables over all projects (runtime metrics on)
OUT/<project>/
  <stem>.txt                   final report per translation unit
  summaries/*.cast-summary.json
  project-summary-index.json
  commands.log                 every command, failure output, warnings
  runtime-metrics/ runtime-duration-*.csv/.tex      (default on)
  prevalence-metrics/*.cast-prevalence.json         (default on)
  partial-*/ partial-status/   sidecars of TUs that failed in C1 (default on)
  failed/ failed-tus.txt       only if something failed
  reports/ paper-facts/        only with --reports / --paper-facts
```

`<stem>` is the source path relative to the source root with `/` replaced by
`_`. Change the base with `--source-root`.

## Reading the result

- Exit code: `0` all good, `1` finished with failures/stale results/skipped
  projects, `2` setup problem (nothing analyzed).
- A TU that fails in C1 is left out of the index and not retried; its output is
  in `failed/` and its partial sidecars in `partial-*`.
- A TU that fails in an import round keeps its previous-phase summary and report,
  prints a red `!!! WARNING: STALE RESULT`, and its report starts with a STALE
  banner. The run summary lists these TUs again.
- `--stop-on-fail` aborts the whole run at the first failing TU.
- `--tu-timeout SEC` kills an analyzer that runs longer than SEC seconds; that TU
  counts as failed ("FAILED (timeout)"). Default: no limit.
- Ctrl-C or `kill` stops the analyzer and its progress helper immediately
  (exit status 130); nothing is left running. Only `kill -9` of the script itself
  can leave the analyzer behind, and its progress helper then exits on its own.
- Slow TUs print a "still running" line every 30 s (`--heartbeat SEC`, `0` = off).

## Common errors

| Message | Fix |
|---|---|
| `cast-history-tool binary not found` | build it (above) or pass `--tool` |
| `no compile_commands.json found` | `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, `bear -- make`, or `--synthesize-compdb` |
| `'stddef.h' file not found` | `--resource-dir "$(clang-NN -print-resource-dir)"` for the tool's LLVM version |
| `resource dir is for LLVM X but the tool was built with LLVM Y` | `--clang clang-Y` |
| `output directory is not empty and was not created by this script` | use a new directory, or `--force` (deletes its contents) |
| entries with a `directory` that does not exist | database generated on another machine: regenerate or `sed` the path prefix |

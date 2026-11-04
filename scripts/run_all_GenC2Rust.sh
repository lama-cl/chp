#!/usr/bin/env bash
set -u

# Run projects through either the multi-TU summary/import workflow or a
# source-only benchmark workflow. Source files can be discovered from
# compile_commands.json, or a temporary compile_commands.json can be generated
# outside the input project for self-contained benchmark sources.
#
# Typical use from the build directory:
#   cd build
#   ../scripts/run_all_multitu_projects.sh
#
# Expected default layout:
#   <tool-root>/
#     build/cast-history-tool
#     scripts/build_summary_index.py
#     test_projects/projects/*/compile_commands.json
#     test_projects/c3a_regressions/*/compile_commands.json
#
# Environment overrides:
#   TOOL=./cast-history-tool
#   TOOL_ROOT=..                         # usually parent of build/
#   PROJECT_ROOT=<tool-root>/test_projects
#   PROJECT_SETS="projects c3a_regressions"
#   PROJECT_GLOB=<custom-dir-containing-projects>
#   PROJECT_PATH=<one-project-or-compile_commands.json>
#   PROJECT_NAME=<output-name-for-PROJECT_PATH>
#   USE_COMPILE_COMMANDS_SOURCES=1       # discover TUs from compile_commands.json
#   RECURSIVE_SOURCE_SCAN=0              # fallback only when not using compile_commands
#   SOURCE_ROOT=<path>                   # optional path prefix for stable output stems
#   SOURCE_EXTENSIONS=".c .cc .cpp"
#   SOURCE_FILTER_REGEX=<python-regex>
#   SOURCE_EXCLUDE_REGEX=<python-regex>
#   REQUIRE_SOURCE_EXISTS=1
#   OUTPUT_ROOT=<tool-root>/test-output/multitu
#   BUILD_INDEX=<tool-root>/scripts/build_summary_index.py
#   PYTHON=python3
#   SUMMARY_CONFIG_ID=<id>
#   SUMMARY_IMPORT_DEBUG=0
#   SUMMARY_IMPORT_ROUNDS=1              # final import/reporting pass after solved index build
#   SUMMARY_REPORTS=0                    # 1: publish verbose *.cast-report.json sidecars
#   PAPER_FACTS=0                        # 1: publish compact *.cast-paper-facts.json sidecars
#   NORMALIZE_PATHS=0
#   PROJECT_KIND=real
#   STOP_ON_FAIL=0
#   KEEP_INTERMEDIATES=0
#   BENCHMARK_SOURCE_MODE=1             # run immediate benchmark subdirectories without input compile_commands.json
#   BENCHMARK_ROOT=<tool-root>/test_projects/bench-genc2rust
#   SYNTHETIC_COMPILE_COMMANDS=1         # create an output-local compile_commands.json when missing
#   SYNTHETIC_CC=clang
#   SYNTHETIC_C_STD=c11
#   SYNTHETIC_CXX_STD=c++17
#   SYNTHETIC_EXTRA_ARGS="..."         # shell-style extra compiler flags for synthesized entries
#   SINGLE_TU_MODE=1                     # analyze every source independently; skip C2/import rounds
#   SOURCE_ONLY_SUMMARY_TMPDIR=/tmp       # native filesystem staging directory for source-only JSON
#   PROGRESS_INTERVAL_SECONDS=30           # runner heartbeat interval for long child commands; 0 disables
#   PROGRESS_SHOW_PROCESS_STATS=1          # include child CPU/RSS when ps supports it
#   ANALYSIS_PROGRESS=0                    # pass --analysis-progress to the analyzer; progress stays on stderr
#   ANALYSIS_PROGRESS_INTERVAL_SECONDS=15  # analyzer progress interval; 0 prints phase transitions only
#   ANALYSIS_PROGRESS_VERBOSE=0             # expensive candidate/attachment diagnostics (small fixtures only)
#   NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK=0 # compare index to legacy membership scan (slow diagnostic)
#   PREVALENCE_METRICS=0                  # emit C6g4w1 raw prevalence sidecars during C1
#   PUBLISH_PARTIAL_C1_SIDECARS=1  # retain C1 JSON/timing sidecars after nonzero analyzer exits
#   PREVALENCE_INCLUDE_RETURN_TYPES=0     # count pointer-typed function return definitions
#
# JSON summary collision avoidance:
#   Each source file is analyzed with its own temporary --summary-dir and the
#   emitted *.cast-summary.json is copied into the round summary directory using
#   the same collision-safe relative-path stem as the text output.  Optional
#   *.cast-report.json sidecars use the same stem but are published to reports/
#   *.cast-paper-facts.json sidecars use the same stem and are published to paper-facts/
#   and never passed to C2.  For example:
#     src/a.c   -> src_a.c.cast-summary.json / src_a.c.cast-report.json
#     tests/a.c -> tests_a.c.cast-summary.json / tests_a.c.cast-report.json
#
# Source-only benchmark mode:
#   BENCHMARK_SOURCE_MODE=1 is intended for corpora such as
#   test_projects/bench-genc2rust where each immediate subdirectory is a
#   benchmark project but no compile_commands.json is supplied. It enables
#   recursive C/C++ source discovery, writes an output-local synthetic
#   compilation database, and enables SINGLE_TU_MODE. Every source file is
#   analyzed independently; no project index or cross-TU summary import is run.
#   Thus, a benchmark directory containing a.c and b.c yields independent
#   a.c.txt / b.c.txt reports and compact a.c.cast-summary.json /
#   b.c.cast-summary.json link summaries. Set SUMMARY_REPORTS=1 to also retain
#   verbose a.c.cast-report.json / b.c.cast-report.json sidecars.
#
# Default output layout for each project:
#   <OUTPUT_ROOT>/<project-name>/
#     <source>.txt                       # final import-aware text output
#     <source>.progress.txt              # optional analyzer stderr/progress sidecar
#     summaries/*.cast-summary.json      # compact C2/import link summaries
#     reports/*.cast-report.json         # optional verbose diagnostics (SUMMARY_REPORTS=1)
#     paper-facts/*.cast-paper-facts.json # optional compact paper facts (PAPER_FACTS=1)
#     runtime-metrics/*.cast-runtime.json # optional compact runtime sidecars (RUNTIME_METRICS=1)
#     runtime-duration-by-tu.csv     # final-stage per-TU timing table
#     runtime-duration-by-project.csv# final-stage project timing table
#     prevalence-metrics/*.cast-prevalence.json  # C1 source-level prevalence sidecars (opt-in)
#     partial-summaries/*.cast-summary.json       # C1 sidecars from failed TUs; never consumed by C2
#     partial-reports/*.cast-report.json          # failed-TU verbose sidecars when SUMMARY_REPORTS=1
#     partial-paper-facts/*.cast-paper-facts.json # failed-TU paper facts when PAPER_FACTS=1
#     partial-runtime-metrics/*.cast-runtime.json # failed-TU timings when RUNTIME_METRICS=1
#     partial-status/*.cast-analysis-status.json  # nonzero C1 exit manifests
#     project-summary-index.json         # index rebuilt from final summaries
#     project-summary-index-used-for-final-import.json
#                                       # index consumed by the final import round
#     import-round-01-index-input.json  # round input indexes are kept at
#     import-round-02-index-input.json  # project root so relative summaryFile
#     ...                               # paths resolve from the same directory
#     commands.log
#
# With KEEP_INTERMEDIATES=1, debug artifacts are also preserved:
#   <OUTPUT_ROOT>/<project-name>/
#     c1-text/*.txt
#     c1-summaries/*.cast-summary.json
#     c1-reports/*.cast-report.json      # when SUMMARY_REPORTS=1
#     c1-paper-facts/*.cast-paper-facts.json # when PAPER_FACTS=1
#     c1-runtime-metrics/*.cast-runtime.json # when RUNTIME_METRICS=1
#     c2-build-index.txt
#     import-round-01-text/*.txt
#     import-round-01-summaries/*.cast-summary.json
#     import-round-01-reports/*.cast-report.json
#     import-round-01-paper-facts/*.cast-paper-facts.json
#     import-round-01-runtime-metrics/*.cast-runtime.json
#     import-round-01-build-index.txt
#     import-round-02-text/*.txt
#     import-round-02-summaries/*.cast-summary.json
#     import-round-02-build-index.txt
#     ...
#
# Note:
#   The script always runs C1 and C2 internally because import-aware passes need
#   per-TU summaries and a project-summary-index.json.  C4G-style solved SCC
#   summaries need at least two import/index refinement rounds by default:
#     C1 summaries -> index0 -> import round 1 -> index1 -> import round 2.
#   Increase SUMMARY_IMPORT_ROUNDS when summary improvements need more rounds to
#   reach callers outside an SCC or to propagate through longer dependency chains.
#   By default intermediate text/log artifacts are temporary and only the latest
#   import-aware outputs are kept.  Import-round index-input JSON files are
#   always placed at the project output root, not under .tmp-intermediates,
#   because SummaryIndex resolves relative summaryFile paths from the index file
#   directory.

TOOL="${TOOL:-./cast-history-tool}"
PYTHON="${PYTHON:-python3}"
SUMMARY_CONFIG_ID="${SUMMARY_CONFIG_ID:-}"
SUMMARY_REPORTS="${SUMMARY_REPORTS:-0}"      # 1: publish verbose *.cast-report.json sidecars
PAPER_FACTS="${PAPER_FACTS:-0}"          # 1: publish compact *.cast-paper-facts.json sidecars
RUNTIME_METRICS="${RUNTIME_METRICS:-0}"  # 1: publish compact runtime sidecars and aggregate tables
SUMMARY_IMPORT_DEBUG="${SUMMARY_IMPORT_DEBUG:-0}"
SUMMARY_IMPORT_ROUNDS="${SUMMARY_IMPORT_ROUNDS:-1}"
NORMALIZE_PATHS="${NORMALIZE_PATHS:-0}"
PROJECT_KIND="${PROJECT_KIND:-real}"
STOP_ON_FAIL="${STOP_ON_FAIL:-0}"
KEEP_INTERMEDIATES="${KEEP_INTERMEDIATES:-0}"
USE_COMPILE_COMMANDS_SOURCES="${USE_COMPILE_COMMANDS_SOURCES:-1}"
RECURSIVE_SOURCE_SCAN="${RECURSIVE_SOURCE_SCAN:-0}"
SOURCE_EXTENSIONS="${SOURCE_EXTENSIONS:-.c .cc .cpp}"
SOURCE_FILTER_REGEX="${SOURCE_FILTER_REGEX:-}"
SOURCE_EXCLUDE_REGEX="${SOURCE_EXCLUDE_REGEX:-}"
REQUIRE_SOURCE_EXISTS="${REQUIRE_SOURCE_EXISTS:-1}"
BENCHMARK_SOURCE_MODE="${BENCHMARK_SOURCE_MODE:-0}"
BENCHMARK_ROOT="${BENCHMARK_ROOT:-}"
SYNTHETIC_COMPILE_COMMANDS="${SYNTHETIC_COMPILE_COMMANDS:-0}"
SYNTHETIC_CC="${SYNTHETIC_CC:-clang}"
SYNTHETIC_C_STD="${SYNTHETIC_C_STD:-c11}"
SYNTHETIC_CXX_STD="${SYNTHETIC_CXX_STD:-c++17}"
SYNTHETIC_EXTRA_ARGS="${SYNTHETIC_EXTRA_ARGS:-}"
SINGLE_TU_MODE="${SINGLE_TU_MODE:-0}"
SOURCE_ONLY_SUMMARY_TMPDIR="${SOURCE_ONLY_SUMMARY_TMPDIR:-${TMPDIR:-/tmp}}"
PROGRESS_INTERVAL_SECONDS="${PROGRESS_INTERVAL_SECONDS:-30}"
PROGRESS_SHOW_PROCESS_STATS="${PROGRESS_SHOW_PROCESS_STATS:-1}"
ANALYSIS_PROGRESS="${ANALYSIS_PROGRESS:-0}"
ANALYSIS_PROGRESS_INTERVAL_SECONDS="${ANALYSIS_PROGRESS_INTERVAL_SECONDS:-15}"
ANALYSIS_PROGRESS_VERBOSE="${ANALYSIS_PROGRESS_VERBOSE:-0}"
NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK="${NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK:-0}"
PREVALENCE_METRICS="${PREVALENCE_METRICS:-0}"
PREVALENCE_INCLUDE_RETURN_TYPES="${PREVALENCE_INCLUDE_RETURN_TYPES:-0}"
PUBLISH_PARTIAL_C1_SIDECARS="${PUBLISH_PARTIAL_C1_SIDECARS:-1}"

# Benchmark-source mode is deliberately source-only. It is appropriate when a
# benchmark suite supplies self-contained C files but no authoritative build
# metadata. In particular, it prevents separate files in one benchmark
# directory from being treated as cross-TU components.
if [[ "$BENCHMARK_SOURCE_MODE" == "1" ]]; then
  USE_COMPILE_COMMANDS_SOURCES=0
  RECURSIVE_SOURCE_SCAN=1
  SYNTHETIC_COMPILE_COMMANDS=1
  SINGLE_TU_MODE=1
fi

export SOURCE_EXTENSIONS SOURCE_FILTER_REGEX SOURCE_EXCLUDE_REGEX REQUIRE_SOURCE_EXISTS

if ! [[ "$SUMMARY_IMPORT_ROUNDS" =~ ^[0-9]+$ ]]; then
  echo "error: SUMMARY_IMPORT_ROUNDS must be a non-negative integer: $SUMMARY_IMPORT_ROUNDS" >&2
  exit 1
fi

if ! [[ "$PROGRESS_INTERVAL_SECONDS" =~ ^[0-9]+$ ]]; then
  echo "error: PROGRESS_INTERVAL_SECONDS must be a non-negative integer: $PROGRESS_INTERVAL_SECONDS" >&2
  exit 1
fi

if ! [[ "$ANALYSIS_PROGRESS_INTERVAL_SECONDS" =~ ^[0-9]+$ ]]; then
  echo "error: ANALYSIS_PROGRESS_INTERVAL_SECONDS must be a non-negative integer: $ANALYSIS_PROGRESS_INTERVAL_SECONDS" >&2
  exit 1
fi

for bool_name in BENCHMARK_SOURCE_MODE SYNTHETIC_COMPILE_COMMANDS SINGLE_TU_MODE PROGRESS_SHOW_PROCESS_STATS ANALYSIS_PROGRESS ANALYSIS_PROGRESS_VERBOSE NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK PREVALENCE_METRICS PREVALENCE_INCLUDE_RETURN_TYPES SUMMARY_REPORTS PAPER_FACTS RUNTIME_METRICS PUBLISH_PARTIAL_C1_SIDECARS; do
  bool_value="${!bool_name}"
  if [[ "$bool_value" != "0" && "$bool_value" != "1" ]]; then
    echo "error: $bool_name must be 0 or 1: $bool_value" >&2
    exit 1
  fi
done

if [[ "$ANALYSIS_PROGRESS_VERBOSE" == "1" && "$ANALYSIS_PROGRESS" != "1" ]]; then
  echo "error: ANALYSIS_PROGRESS_VERBOSE=1 requires ANALYSIS_PROGRESS=1" >&2
  exit 1
fi

if [[ "$SINGLE_TU_MODE" == "1" && "$SUMMARY_IMPORT_ROUNDS" != "0" ]]; then
  echo "note: SINGLE_TU_MODE=1 ignores SUMMARY_IMPORT_ROUNDS=$SUMMARY_IMPORT_ROUNDS" >&2
fi

if [[ ! -x "$TOOL" ]]; then
  echo "error: tool not found or not executable: $TOOL" >&2
  echo "hint: run from the build directory or set TOOL=/path/to/cast-history-tool" >&2
  exit 1
fi

TOOL_ROOT_INPUT="${TOOL_ROOT:-..}"
if ! TOOL_ROOT_ABS="$(cd "$TOOL_ROOT_INPUT" && pwd -P)"; then
  echo "error: TOOL_ROOT not found: $TOOL_ROOT_INPUT" >&2
  exit 1
fi

PROJECT_ROOT="${PROJECT_ROOT:-$TOOL_ROOT_ABS/test_projects}"
OUTPUT_ROOT="${OUTPUT_ROOT:-$TOOL_ROOT_ABS/test-output/multitu}"
BUILD_INDEX="${BUILD_INDEX:-$TOOL_ROOT_ABS/scripts/build_summary_index.py}"
RUNTIME_AGGREGATOR="${RUNTIME_AGGREGATOR:-$TOOL_ROOT_ABS/scripts/aggregate_runtime_metrics.py}"
PROJECT_SETS="${PROJECT_SETS:-projects c3a_regressions c4a_dependency_edges}"

if [[ "$SINGLE_TU_MODE" != "1" && ! -f "$BUILD_INDEX" ]]; then
  echo "error: C2 index builder not found: $BUILD_INDEX" >&2
  echo "hint: copy scripts/build_summary_index.py into the repo or set BUILD_INDEX=/path/to/build_summary_index.py" >&2
  exit 1
fi

if [[ "$RUNTIME_METRICS" == "1" && ! -f "$RUNTIME_AGGREGATOR" ]]; then
  echo "error: runtime timing aggregator not found: $RUNTIME_AGGREGATOR" >&2
  echo "hint: copy scripts/aggregate_runtime_metrics.py into the repo or set RUNTIME_AGGREGATOR=/path/to/aggregate_runtime_metrics.py" >&2
  exit 1
fi

mkdir -p "$OUTPUT_ROOT"

sanitize_name() {
  local s="$1"
  s="${s//\\//}"
  s="${s//\//_}"
  s="${s// /_}"
  s="${s//:/_}"
  printf '%s' "$s" | sed -e 's/[^A-Za-z0-9._-]/_/g' -e 's/__*/_/g' -e 's/^_//' -e 's/_$//'
}

escape_sed() {
  printf '%s' "$1" | sed -e 's/[\/&]/\\&/g'
}

TOOL_ROOT_ESC="$(escape_sed "$TOOL_ROOT_ABS")"
PROJECT_ROOT_ABS="$(cd "$PROJECT_ROOT" 2>/dev/null && pwd -P || printf '%s' "$PROJECT_ROOT")"
PROJECT_ROOT_ESC="$(escape_sed "$PROJECT_ROOT_ABS")"
# Clang Tooling may execute a translation unit relative to the compilation
# database directory.  Keep every tool-facing output path absolute so an
# OUTPUT_ROOT supplied relative to the launch directory cannot be relocated
# beneath the benchmark project (notably prevalence sidecars in source-only
# mode).
OUTPUT_ROOT_ABS="$(cd "$OUTPUT_ROOT" && pwd -P)"
OUTPUT_ROOT="$OUTPUT_ROOT_ABS"
OUTPUT_ROOT_ESC="$(escape_sed "$OUTPUT_ROOT_ABS")"

normalize_output() {
  if [[ "$NORMALIZE_PATHS" == "1" ]]; then
    sed \
      -e "s/${TOOL_ROOT_ESC}/<tool-root>/g" \
      -e "s/${PROJECT_ROOT_ESC}/<test-projects>/g" \
      -e "s/${OUTPUT_ROOT_ESC}/<output-root>/g"
  else
    cat
  fi
}

abs_path() {
  local p="$1"
  if [[ -d "$p" ]]; then
    (cd "$p" && pwd -P)
  else
    local d
    d="$(dirname "$p")"
    local b
    b="$(basename "$p")"
    (cd "$d" && printf '%s/%s\n' "$(pwd -P)" "$b")
  fi
}

compile_commands_for_project() {
  local project="$1"
  if [[ -f "$project" && "$(basename "$project")" == "compile_commands.json" ]]; then
    printf '%s\n' "$project"
  elif [[ -f "$project/compile_commands.json" ]]; then
    printf '%s/compile_commands.json\n' "$project"
  fi
}

project_dir_for_tool() {
  local project="$1"
  if [[ -f "$project" && "$(basename "$project")" == "compile_commands.json" ]]; then
    dirname "$project"
  else
    printf '%s\n' "$project"
  fi
}

collect_projects() {
  local roots=()

  # PROJECT_PATH is for running one real project/build directory, or one
  # compile_commands.json file, without wrapping it in test_projects/projects/.
  if [[ -n "${PROJECT_PATH:-}" ]]; then
    if [[ -f "$PROJECT_PATH" && "$(basename "$PROJECT_PATH")" == "compile_commands.json" ]]; then
      dirname "$PROJECT_PATH"
      return
    fi
    if [[ -d "$PROJECT_PATH" ]]; then
      printf '%s\n' "$PROJECT_PATH"
      return
    fi
    echo "warning: PROJECT_PATH not found, skipping: $PROJECT_PATH" >&2
    return
  fi

  if [[ "$BENCHMARK_SOURCE_MODE" == "1" ]]; then
    local benchmark_root="$BENCHMARK_ROOT"
    if [[ -z "$benchmark_root" ]]; then
      benchmark_root="$TOOL_ROOT_ABS/test_projects/bench-genc2rust"
    fi
    if [[ ! -d "$benchmark_root" ]]; then
      echo "warning: BENCHMARK_ROOT not found, skipping: $benchmark_root" >&2
      return
    fi
    # Each immediate child is one benchmark project. Source discovery below is
    # recursive, so the exceptional benchmark with several C files is handled
    # as independent source-only analyses.
    find "$benchmark_root" -mindepth 1 -maxdepth 1 -type d | sort
    return
  fi

  if [[ -n "${PROJECT_GLOB:-}" ]]; then
    roots+=("$PROJECT_GLOB")
  else
    for set_name in $PROJECT_SETS; do
      roots+=("$PROJECT_ROOT/$set_name")
    done
  fi

  for root in "${roots[@]}"; do
    if [[ ! -d "$root" ]]; then
      echo "warning: project set not found, skipping: $root" >&2
      continue
    fi

    # If the supplied root itself is a project/build directory, run it directly.
    if [[ -f "$root/compile_commands.json" ]]; then
      printf '%s\n' "$root"
    else
      find "$root" -mindepth 1 -maxdepth 1 -type d | sort
    fi
  done
}

collect_sources_from_compile_commands() {
  local compdb="$1"
  "$PYTHON" - "$compdb" <<'PY'
import json
import os
import re
import sys

compdb = sys.argv[1]
compdb_dir = os.path.dirname(os.path.abspath(compdb))
exts = tuple(e.lower() for e in os.environ.get("SOURCE_EXTENSIONS", ".c .cc .cpp").split())
filter_re = os.environ.get("SOURCE_FILTER_REGEX", "")
exclude_re = os.environ.get("SOURCE_EXCLUDE_REGEX", "")
require_exists = os.environ.get("REQUIRE_SOURCE_EXISTS", "1") != "0"

try:
    with open(compdb, "r", encoding="utf-8") as f:
        entries = json.load(f)
except Exception as exc:
    print(f"error: failed to read compile_commands.json {compdb}: {exc}", file=sys.stderr)
    sys.exit(2)

if not isinstance(entries, list):
    print(f"error: compile_commands.json is not a list: {compdb}", file=sys.stderr)
    sys.exit(2)

filter_pat = re.compile(filter_re) if filter_re else None
exclude_pat = re.compile(exclude_re) if exclude_re else None
seen = set()
sources = []
for entry in entries:
    if not isinstance(entry, dict):
        continue
    file_name = entry.get("file")
    if not file_name:
        continue
    directory = entry.get("directory") or compdb_dir
    if not os.path.isabs(file_name):
        file_name = os.path.join(directory, file_name)
    file_name = os.path.normpath(os.path.abspath(file_name))
    if exts and not file_name.lower().endswith(exts):
        continue
    if filter_pat and not filter_pat.search(file_name):
        continue
    if exclude_pat and exclude_pat.search(file_name):
        continue
    if require_exists and not os.path.exists(file_name):
        continue
    if file_name not in seen:
        seen.add(file_name)
        sources.append(file_name)

for path in sorted(sources):
    print(path)
PY
}

generate_synthetic_compile_commands() {
  local out_dir="$1"
  local project_dir="$2"
  shift 2
  local out_file="$out_dir/compile_commands.json"

  mkdir -p "$out_dir"
  "$PYTHON" - "$out_file" "$project_dir" "$SYNTHETIC_CC" "$SYNTHETIC_C_STD" "$SYNTHETIC_CXX_STD" "$@" <<'PY_COMPDB'
import json
import os
import shlex
import sys

out_file, project_dir, compiler, c_std, cxx_std, *sources = sys.argv[1:]
project_dir = os.path.abspath(project_dir)
extra_args = shlex.split(os.environ.get("SYNTHETIC_EXTRA_ARGS", ""))
entries = []
for source in sources:
    source = os.path.abspath(source)
    suffix = os.path.splitext(source)[1].lower()
    std = c_std if suffix == ".c" else cxx_std
    entries.append({
        "directory": project_dir,
        "file": source,
        "arguments": [
            compiler,
            f"-std={std}",
            "-I.",
            *extra_args,
            "-c",
            source,
            "-o",
            os.devnull,
        ],
    })
with open(out_file, "w", encoding="utf-8") as f:
    json.dump(entries, f, indent=2)
    f.write("\n")
PY_COMPDB
}

collect_sources() {
  local project="$1"
  local project_dir
  project_dir="$(project_dir_for_tool "$project")"
  local compdb
  compdb="$(compile_commands_for_project "$project")"

  if [[ "$USE_COMPILE_COMMANDS_SOURCES" == "1" && -n "$compdb" && -f "$compdb" ]]; then
    collect_sources_from_compile_commands "$compdb"
    return
  fi

  if [[ "$RECURSIVE_SOURCE_SCAN" == "1" ]]; then
    find "$project_dir" -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) | sort
  else
    find "$project_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) | sort
  fi
}

source_stem() {
  local project="$1"
  local src="$2"
  local src_abs
  src_abs="$(abs_path "$src")"
  local root_abs=""
  local rel="$src_abs"

  if [[ -n "${SOURCE_ROOT:-}" && -d "$SOURCE_ROOT" ]]; then
    root_abs="$(abs_path "$SOURCE_ROOT")"
  else
    local project_dir
    project_dir="$(project_dir_for_tool "$project")"
    root_abs="$(abs_path "$project_dir")"
  fi

  if [[ -n "$root_abs" && "$src_abs" == "$root_abs/"* ]]; then
    rel="${src_abs#$root_abs/}"
  fi

  sanitize_name "$rel"
}

format_elapsed() {
  local total_seconds="$1"
  local hours=$((total_seconds / 3600))
  local minutes=$(((total_seconds % 3600) / 60))
  local seconds=$((total_seconds % 60))
  printf '%02d:%02d:%02d' "$hours" "$minutes" "$seconds"
}

process_stats_for_pid() {
  local pid="$1"
  if [[ "$PROGRESS_SHOW_PROCESS_STATS" != "1" ]] || ! command -v ps >/dev/null 2>&1; then
    return 0
  fi

  ps -p "$pid" -o pcpu= -o rss= 2>/dev/null |
    awk 'NF { printf "cpu=%s%%; rss=%s KiB", $1, $2; exit }'
}

progress_watch_child() {
  local child_pid="$1"
  local label="$2"
  local tmp="$3"
  local started_seconds="$4"
  local previous_size=0
  local last_output_change_seconds="$started_seconds"

  while sleep "$PROGRESS_INTERVAL_SECONDS"; do
    if ! kill -0 "$child_pid" 2>/dev/null; then
      return 0
    fi

    local now_seconds=$SECONDS
    local elapsed_seconds=$((now_seconds - started_seconds))
    local current_size=0
    if [[ -f "$tmp" ]]; then
      current_size="$(wc -c <"$tmp" | tr -d '[:space:]')"
    fi
    if [[ "$current_size" != "$previous_size" ]]; then
      previous_size="$current_size"
      last_output_change_seconds="$now_seconds"
    fi

    local stats
    stats="$(process_stats_for_pid "$child_pid")"
    local stats_fragment=""
    if [[ -n "$stats" ]]; then
      stats_fragment="; $stats"
    fi

    local output_idle_seconds=$((now_seconds - last_output_change_seconds))
    printf '      [progress] %s: elapsed %s; pid %s%s; captured output=%s bytes; no new output for %s\n' \
      "$label" \
      "$(format_elapsed "$elapsed_seconds")" \
      "$child_pid" \
      "$stats_fragment" \
      "$current_size" \
      "$(format_elapsed "$output_idle_seconds")"
  done
}

# By default, reports preserve the historical behavior of capturing stdout and
# stderr together. When RUN_SEPARATE_STDERR=1 (used for --analysis-progress),
# stdout remains the only content of the cast-history report and stderr is
# streamed live to the terminal and saved next to the report as .stderr.txt.
# This keeps execution telemetry out of the semantic report while preserving
# diagnostics for later inspection.
run_and_capture() {
  local out_file="$1"
  shift
  local tmp="${out_file}.tmp"
  local progress_label="${RUN_PROGRESS_LABEL:-$(basename "$out_file")}"
  local separate_stderr="${RUN_SEPARATE_STDERR:-0}"
  local stderr_file="${out_file}.stderr.txt"
  local started_seconds=$SECONDS
  local child_pid=""
  local watcher_pid=""
  local status=0

  mkdir -p "$(dirname "$out_file")"
  if [[ "$separate_stderr" == "1" ]]; then
    : >"$stderr_file"
  else
    rm -f "$stderr_file"
  fi

  (
    {
      echo "Command:"
      printf '%q ' "$@"
      echo
      echo
      echo "Output:"
    } >"$tmp"

    if [[ "$separate_stderr" == "1" ]]; then
      # tee mirrors analyzer progress/diagnostics to the terminal and writes a
      # sidecar log, while stdout remains the main cast-history report.
      exec "$@" >>"$tmp" 2> >(tee "$stderr_file" >&2)
    else
      exec "$@" >>"$tmp" 2>&1
    fi
  ) &
  child_pid=$!

  if [[ "$PROGRESS_INTERVAL_SECONDS" != "0" ]]; then
    progress_watch_child "$child_pid" "$progress_label" "$tmp" "$started_seconds" &
    watcher_pid=$!
  fi

  wait "$child_pid"
  status=$?

  if [[ -n "$watcher_pid" ]]; then
    kill "$watcher_pid" 2>/dev/null || true
    wait "$watcher_pid" 2>/dev/null || true
  fi

  normalize_output <"$tmp" >"$out_file"
  rm -f "$tmp"
  return $status
}

append_file_to_log() {
  local log_file="$1"
  local label="$2"
  local src_file="$3"
  {
    echo
    echo "===== $label ====="
    if [[ -f "$src_file" ]]; then
      cat "$src_file"
    else
      echo "<missing: $src_file>"
    fi
  } >>"$log_file"
}

# Copy the summary emitted for one source from an isolated temporary summary
# directory into the round's aggregate summary directory using a collision-safe
# stem.  This prevents two files such as src/a.c and tests/a.c from overwriting
# one another if the analyzer emits basename-derived files like a.c.cast-summary.json.
publish_source_summaries() {
  local src_summary_dir="$1"
  local dest_summary_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_summary_dir"
  local summaries=()
  if [[ -d "$src_summary_dir" ]]; then
    mapfile -t summaries < <(find "$src_summary_dir" -type f -name '*.cast-summary.json' | sort)
  fi

  if [[ ${#summaries[@]} -eq 0 ]]; then
    {
      echo
      echo "===== ${label} summary publish warning ====="
      echo "no *.cast-summary.json emitted under: $src_summary_dir"
      echo "expected destination stem: $stem"
    } >>"$log_file"
    return 1
  fi

  local dest="$dest_summary_dir/${stem}.cast-summary.json"
  cp "${summaries[0]}" "$dest"

  if [[ ${#summaries[@]} -gt 1 ]]; then
    local i=1
    local extra
    for extra in "${summaries[@]:1}"; do
      cp "$extra" "$dest_summary_dir/${stem}__extra_${i}.cast-summary.json"
      ((i++))
    done
    {
      echo
      echo "===== ${label} summary publish warning ====="
      echo "multiple summaries emitted for one source; copied first to: $dest"
      echo "extra summaries copied with __extra_N suffixes"
      printf 'source summary dir: %s\n' "$src_summary_dir"
    } >>"$log_file"
  fi

  return 0
}

# Source-only benchmark runs analyze exactly one source per tool invocation.
# Requesting an exact --summary-json path is more robust than discovering a
# source-derived filename below --summary-dir, and it avoids any dependence on
# the analyzer's summary-dir naming convention.
publish_explicit_summary() {
  local expected_summary="$1"
  local dest_summary_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_summary_dir"
  if [[ ! -f "$expected_summary" ]]; then
    {
      echo
      echo "===== ${label} summary publish warning ====="
      echo "expected explicit summary JSON was not emitted: $expected_summary"
      echo "summary parent directory contents:"
      if [[ -d "$(dirname "$expected_summary")" ]]; then
        find "$(dirname "$expected_summary")" -maxdepth 2 -type f -printf '  %p\n' | sort
      else
        echo "  <missing directory>"
      fi
    } >>"$log_file"
    return 1
  fi

  local dest="$dest_summary_dir/${stem}.cast-summary.json"
  if ! cp "$expected_summary" "$dest"; then
    {
      echo
      echo "===== ${label} summary publish warning ====="
      echo "could not copy explicit summary JSON"
      echo "source: $expected_summary"
      echo "destination: $dest"
    } >>"$log_file"
    return 1
  fi
  return 0
}
# C6g5e: reports are optional and never enter the summary/import directory.
publish_source_reports() {
  local src_report_dir="$1"
  local dest_report_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_report_dir"
  local reports=()
  if [[ -d "$src_report_dir" ]]; then
    mapfile -t reports < <(find "$src_report_dir" -type f -name '*.cast-report.json' | sort)
  fi

  if [[ ${#reports[@]} -eq 0 ]]; then
    {
      echo
      echo "===== ${label} report publish warning ====="
      echo "no *.cast-report.json emitted under: $src_report_dir"
      echo "expected destination stem: $stem"
    } >>"$log_file"
    return 1
  fi

  local dest="$dest_report_dir/${stem}.cast-report.json"
  cp "${reports[0]}" "$dest"
  if [[ ${#reports[@]} -gt 1 ]]; then
    local i=1
    local extra
    for extra in "${reports[@]:1}"; do
      cp "$extra" "$dest_report_dir/${stem}__extra_${i}.cast-report.json"
      ((i++))
    done
  fi
  return 0
}

publish_explicit_report() {
  local expected_report="$1"
  local dest_report_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_report_dir"
  if [[ ! -f "$expected_report" ]]; then
    {
      echo
      echo "===== ${label} report publish warning ====="
      echo "expected explicit report JSON was not emitted: $expected_report"
    } >>"$log_file"
    return 1
  fi

  local dest="$dest_report_dir/${stem}.cast-report.json"
  if ! cp "$expected_report" "$dest"; then
    {
      echo
      echo "===== ${label} report publish warning ====="
      echo "could not copy explicit report JSON"
      echo "source: $expected_report"
      echo "destination: $dest"
    } >>"$log_file"
    return 1
  fi
  return 0
}


# C6g5f: compact paper facts are independent of reports and never enter C2.
publish_source_paper_facts() {
  local src_dir="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_dir"
  local facts=()
  if [[ -d "$src_dir" ]]; then
    mapfile -t facts < <(find "$src_dir" -type f -name '*.cast-paper-facts.json' | sort)
  fi
  if [[ ${#facts[@]} -eq 0 ]]; then
    {
      echo
      echo "===== ${label} paper-facts publish warning ====="
      echo "no *.cast-paper-facts.json emitted under: $src_dir"
      echo "expected destination stem: $stem"
    } >>"$log_file"
    return 1
  fi
  local dest="$dest_dir/${stem}.cast-paper-facts.json"
  cp "${facts[0]}" "$dest"
  if [[ ${#facts[@]} -gt 1 ]]; then
    local i=1
    local extra
    for extra in "${facts[@]:1}"; do
      cp "$extra" "$dest_dir/${stem}__extra_${i}.cast-paper-facts.json"
      ((i++))
    done
  fi
  return 0
}

publish_explicit_paper_facts() {
  local expected="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"
  mkdir -p "$dest_dir"
  if [[ ! -f "$expected" ]]; then
    {
      echo
      echo "===== ${label} paper-facts publish warning ====="
      echo "expected explicit paper-facts JSON was not emitted: $expected"
    } >>"$log_file"
    return 1
  fi
  local dest="$dest_dir/${stem}.cast-paper-facts.json"
  if ! cp "$expected" "$dest"; then
    {
      echo
      echo "===== ${label} paper-facts publish warning ====="
      echo "could not copy explicit paper-facts JSON"
      echo "source: $expected"
      echo "destination: $dest"
    } >>"$log_file"
    return 1
  fi
  return 0
}

# C6g5g: compact runtime timing sidecars are paper-performance artifacts.
# They are never passed to C2 and only final-stage files are published under
# runtime-metrics/.
publish_source_runtime_metrics() {
  local src_dir="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_dir"
  local timings=()
  if [[ -d "$src_dir" ]]; then
    mapfile -t timings < <(find "$src_dir" -type f -name '*.cast-runtime.json' | sort)
  fi
  if [[ ${#timings[@]} -eq 0 ]]; then
    {
      echo
      echo "===== ${label} runtime-timing publish warning ====="
      echo "no *.cast-runtime.json emitted under: $src_dir"
      echo "expected destination stem: $stem"
    } >>"$log_file"
    return 1
  fi
  local dest="$dest_dir/${stem}.cast-runtime.json"
  cp "${timings[0]}" "$dest"
  if [[ ${#timings[@]} -gt 1 ]]; then
    local i=1
    local extra
    for extra in "${timings[@]:1}"; do
      cp "$extra" "$dest_dir/${stem}__extra_${i}.cast-runtime.json"
      ((i++))
    done
  fi
  return 0
}

publish_explicit_runtime_metrics() {
  local expected="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"
  mkdir -p "$dest_dir"
  if [[ ! -f "$expected" ]]; then
    {
      echo
      echo "===== ${label} runtime-timing publish warning ====="
      echo "expected explicit runtime timing JSON was not emitted: $expected"
    } >>"$log_file"
    return 1
  fi
  local dest="$dest_dir/${stem}.cast-runtime.json"
  if ! cp "$expected" "$dest"; then
    {
      echo
      echo "===== ${label} runtime-timing publish warning ====="
      echo "could not copy explicit runtime timing JSON"
      echo "source: $expected"
      echo "destination: $dest"
    } >>"$log_file"
    return 1
  fi
  return 0
}


# Publish sidecars from a failed C1 invocation without making them eligible for
# C2.  The analyzer can finish HandleTranslationUnit and write sidecars even
# when Clang returns a nonzero frontend status.  These helpers preserve those
# files under partial-* and record an explicit status manifest instead of
# mixing them into normal summaries/, reports/, or runtime-metrics/ outputs.
publish_partial_file_if_present() {
  local source_file="$1"
  local dest_dir="$2"
  local stem="$3"
  local suffix="$4"
  local artifact_kind="$5"
  local log_file="$6"
  local label="$7"

  [[ -f "$source_file" ]] || return 1
  mkdir -p "$dest_dir"
  local dest="$dest_dir/${stem}${suffix}"
  if ! cp "$source_file" "$dest"; then
    {
      echo
      echo "===== ${label} partial sidecar publish warning ====="
      echo "could not copy partial ${artifact_kind} sidecar"
      echo "source: $source_file"
      echo "destination: $dest"
    } >>"$log_file"
    return 2
  fi

  PARTIAL_PUBLISHED_ARTIFACTS+=("${artifact_kind}|$(basename "$dest_dir")/$(basename "$dest")")
  return 0
}

publish_partial_discovered_sidecar_if_present() {
  local source_dir="$1"
  local filename_pattern="$2"
  local dest_dir="$3"
  local stem="$4"
  local suffix="$5"
  local artifact_kind="$6"
  local log_file="$7"
  local label="$8"

  local files=()
  if [[ -d "$source_dir" ]]; then
    mapfile -t files < <(find "$source_dir" -type f -name "$filename_pattern" | sort)
  fi
  [[ ${#files[@]} -gt 0 ]] || return 1

  mkdir -p "$dest_dir"
  local dest="$dest_dir/${stem}${suffix}"
  if ! cp "${files[0]}" "$dest"; then
    {
      echo
      echo "===== ${label} partial sidecar publish warning ====="
      echo "could not copy partial ${artifact_kind} sidecar"
      echo "source: ${files[0]}"
      echo "destination: $dest"
    } >>"$log_file"
    return 2
  fi

  if [[ ${#files[@]} -gt 1 ]]; then
    local i=1
    local extra
    for extra in "${files[@]:1}"; do
      cp "$extra" "$dest_dir/${stem}__extra_${i}${suffix}" || true
      ((i++))
    done
    {
      echo
      echo "===== ${label} partial sidecar publish warning ====="
      echo "multiple ${artifact_kind} sidecars emitted for one failed source; copied first to: $dest"
      echo "extra sidecars copied with __extra_N suffixes"
    } >>"$log_file"
  fi

  PARTIAL_PUBLISHED_ARTIFACTS+=("${artifact_kind}|$(basename "$dest_dir")/$(basename "$dest")")
  return 0
}

write_partial_c1_status_manifest() {
  local status_file="$1"
  local source_file="$2"
  local source_stem="$3"
  local tool_exit_code="$4"
  local text_file_name="$5"
  local runner_name="$6"
  shift 6

  mkdir -p "$(dirname "$status_file")"
  "$PYTHON" - "$status_file" "$source_file" "$source_stem" \
      "$tool_exit_code" "$text_file_name" "$runner_name" "$@" <<'PY_STATUS'
import json
import pathlib
import sys

status_path = pathlib.Path(sys.argv[1])
source_file = sys.argv[2]
source_stem = sys.argv[3]
exit_code = int(sys.argv[4])
text_file_name = sys.argv[5]
runner_name = sys.argv[6]
artifacts = []
for item in sys.argv[7:]:
    kind, sep, path = item.partition('|')
    if not sep:
        continue
    artifacts.append({"kind": kind, "path": path})

payload = {
    "schemaVersion": 1,
    "sourceFile": source_file,
    "sourceStem": source_stem,
    "stage": "c1",
    "complete": False,
    "c2Eligible": False,
    "toolExitCode": exit_code,
    "runner": runner_name,
    "capturedTextFile": text_file_name,
    "reason": "analyzer exited nonzero; any emitted C1 sidecars were published as partial artifacts",
    "publishedArtifacts": artifacts,
}
status_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY_STATUS
}

mapfile -t projects < <(collect_projects)

if [[ ${#projects[@]} -eq 0 ]]; then
  echo "warning: no multi-TU projects found under $PROJECT_ROOT" >&2
  exit 0
fi

passed=0
failed=0
skipped=0

echo "Running ${#projects[@]} project(s)"
echo "  tool:                 $TOOL"
echo "  tool root:            $TOOL_ROOT_ABS"
echo "  project root:         $PROJECT_ROOT"
echo "  project sets:         $PROJECT_SETS"
if [[ -n "${PROJECT_PATH:-}" ]]; then
  echo "  project path:         $PROJECT_PATH"
fi
echo "  output root:          $OUTPUT_ROOT"
if [[ "$SINGLE_TU_MODE" == "1" ]]; then
  echo "  C2 index builder:     <skipped in SINGLE_TU_MODE>"
else
  echo "  C2 index builder:     $BUILD_INDEX"
fi
echo "  project kind:         $PROJECT_KIND"
echo "  summary import debug: $SUMMARY_IMPORT_DEBUG"
echo "  import rounds:        $SUMMARY_IMPORT_ROUNDS"
echo "  verbose reports:      $SUMMARY_REPORTS"
echo "  compact paper facts:  $PAPER_FACTS"
echo "  runtime metrics:      $RUNTIME_METRICS"
echo "  partial C1 sidecars:  $PUBLISH_PARTIAL_C1_SIDECARS"
if [[ "$RUNTIME_METRICS" == "1" ]]; then
  echo "  runtime aggregator:   $RUNTIME_AGGREGATOR"
fi
echo "  normalized paths:     $NORMALIZE_PATHS"
echo "  keep intermediates:   $KEEP_INTERMEDIATES"
echo "  source discovery:     $([[ "$USE_COMPILE_COMMANDS_SOURCES" == "1" ]] && echo compile_commands || echo source-scan)"
echo "  recursive fallback:   $RECURSIVE_SOURCE_SCAN"
echo "  source extensions:    $SOURCE_EXTENSIONS"
echo "  benchmark source mode:$BENCHMARK_SOURCE_MODE"
echo "  synthetic compdb:     $SYNTHETIC_COMPILE_COMMANDS"
echo "  single-TU mode:       $SINGLE_TU_MODE"
echo "  runner progress:      ${PROGRESS_INTERVAL_SECONDS}s (0 disables)"
echo "  process stats:        $PROGRESS_SHOW_PROCESS_STATS"
echo "  analyzer progress:    $ANALYSIS_PROGRESS"
echo "  analyzer verbose:     $ANALYSIS_PROGRESS_VERBOSE"
echo "  pointer-use index cross-check: $NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK"
echo "  prevalence metrics:  $PREVALENCE_METRICS"
if [[ "$PREVALENCE_METRICS" == "1" ]]; then
  echo "  prevalence returns:  $PREVALENCE_INCLUDE_RETURN_TYPES"
fi
if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
  echo "  analyzer interval:    ${ANALYSIS_PROGRESS_INTERVAL_SECONDS}s (0 = phase transitions only)"
fi
if [[ "$SINGLE_TU_MODE" == "1" ]]; then
  echo "  JSON staging dir:     $SOURCE_ONLY_SUMMARY_TMPDIR"
fi
if [[ "$BENCHMARK_SOURCE_MODE" == "1" ]]; then
  echo "  benchmark root:       ${BENCHMARK_ROOT:-$TOOL_ROOT_ABS/test_projects/bench-genc2rust}"
fi
if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
  echo "  config id:            $SUMMARY_CONFIG_ID"
fi
echo

for project in "${projects[@]}"; do
  if [[ -n "${PROJECT_NAME:-}" && ${#projects[@]} -eq 1 ]]; then
    name="$PROJECT_NAME"
  else
    name="$(basename "$(project_dir_for_tool "$project")")"
  fi
  safe="$(sanitize_name "$name")"
  project_out="$OUTPUT_ROOT/$safe"

  # Final/default outputs.
  final_text="$project_out"
  final_summaries="$project_out/summaries"
  final_reports="$project_out/reports"
  final_paper_facts="$project_out/paper-facts"
  final_runtime_metrics="$project_out/runtime-metrics"
  prevalence_metrics_dir="$project_out/prevalence-metrics"
  partial_summaries="$project_out/partial-summaries"
  partial_reports="$project_out/partial-reports"
  partial_paper_facts="$project_out/partial-paper-facts"
  partial_runtime_metrics="$project_out/partial-runtime-metrics"
  partial_status="$project_out/partial-status"
  index_file="$project_out/project-summary-index.json"
  final_import_index="$project_out/project-summary-index-used-for-final-import.json"
  log_file="$project_out/commands.log"

  # Intermediate/debug outputs.
  if [[ "$KEEP_INTERMEDIATES" == "1" ]]; then
    tmp_root=""
    c1_text="$project_out/c1-text"
    c1_summaries="$project_out/c1-summaries"
    c1_reports="$project_out/c1-reports"
    c1_paper_facts="$project_out/c1-paper-facts"
    c1_runtime_metrics="$project_out/c1-runtime-metrics"
    c1_summary_tmp_root="$project_out/c1-summary-tmp"
    c2_txt="$project_out/c2-build-index.txt"
    rounds_root="$project_out"
  else
    tmp_root="$project_out/.tmp-intermediates"
    c1_text="$tmp_root/c1-text"
    c1_summaries="$tmp_root/c1-summaries"
    c1_reports="$tmp_root/c1-reports"
    c1_paper_facts="$tmp_root/c1-paper-facts"
    c1_runtime_metrics="$tmp_root/c1-runtime-metrics"
    c1_summary_tmp_root="$tmp_root/c1-summary-tmp"
    c2_txt="$tmp_root/c2-build-index.txt"
    rounds_root="$tmp_root"
  fi

  rm -rf "$project_out"
  mkdir -p "$final_text" "$final_summaries" "$prevalence_metrics_dir" \
           "$c1_text" "$c1_summaries" "$c1_summary_tmp_root" "$rounds_root"
  if [[ "$SUMMARY_REPORTS" == "1" ]]; then
    mkdir -p "$final_reports" "$c1_reports"
  fi
  if [[ "$PAPER_FACTS" == "1" ]]; then
    mkdir -p "$final_paper_facts" "$c1_paper_facts"
  fi
  if [[ "$RUNTIME_METRICS" == "1" ]]; then
    mkdir -p "$final_runtime_metrics" "$c1_runtime_metrics"
  fi
  : >"$log_file"

  # The analyzer can emit very large JSON summaries. In source-only mode,
  # stage explicit summary JSON on a native temporary filesystem (normally /tmp)
  # and copy it into OUTPUT_ROOT only after the analyzer closes the file.
  source_only_summary_tmp_root=""
  if [[ "$SINGLE_TU_MODE" == "1" ]]; then
    if ! source_only_summary_tmp_root="$(mktemp -d "${SOURCE_ONLY_SUMMARY_TMPDIR%/}/cast-history-source-only.${safe}.XXXXXX")"; then
      echo "==> $name: FAIL (could not create local source-only JSON staging directory)"
      ((failed++))
      continue
    fi
    {
      echo
      echo "===== source-only JSON staging ====="
      echo "staging directory: $source_only_summary_tmp_root"
      echo "final summaries directory: $final_summaries"
      echo "final paper-facts directory: $final_paper_facts"
      echo "final runtime metrics directory: $final_runtime_metrics"
    } >>"$log_file"
  fi

  project_dir="$(project_dir_for_tool "$project")"
  metrics_project_root="$(abs_path "$project_dir")"
  compdb="$(compile_commands_for_project "$project")"

  mapfile -t sources < <(collect_sources "$project")
  if [[ ${#sources[@]} -eq 0 ]]; then
    echo "==> $name: SKIP (no source files)"
    ((skipped++))
    continue
  fi

  if [[ -z "$compdb" || ! -f "$compdb" ]]; then
    if [[ "$SYNTHETIC_COMPILE_COMMANDS" != "1" ]]; then
      echo "==> $name: SKIP (no compile_commands.json; set SYNTHETIC_COMPILE_COMMANDS=1 for source-only benchmarks)"
      ((skipped++))
      continue
    fi
    compdb_dir="$project_out/synthetic-compdb"
    echo "    generating synthetic compile_commands.json"
    if ! generate_synthetic_compile_commands "$compdb_dir" "$project_dir" "${sources[@]}"; then
      echo "==> $name: FAIL (could not generate synthetic compile_commands.json)"
      ((failed++))
      continue
    fi
    compdb="$compdb_dir/compile_commands.json"
    {
      echo
      echo "===== synthetic compile_commands.json ====="
      echo "generated for source-only analysis; no cross-TU import is implied"
      echo "path: $compdb"
      echo "compiler: $SYNTHETIC_CC"
      echo "C standard: $SYNTHETIC_C_STD"
      echo "C++ standard: $SYNTHETIC_CXX_STD"
      echo "extra args: $SYNTHETIC_EXTRA_ARGS"
      cat "$compdb"
    } >>"$log_file"
  fi
  compdb_dir="$(dirname "$compdb")"

  echo "==> $name"
  echo "    project: $project"
  echo "    sources: ${#sources[@]}"
  echo "    output:  $project_out"

  project_failed=0
  latest_text="$c1_text"
  latest_summaries="$c1_summaries"
  latest_reports="$c1_reports"
  latest_paper_facts="$c1_paper_facts"
  latest_runtime_metrics="$c1_runtime_metrics"
  index_for_next_round="$index_file"
  last_import_index=""
  completed_import_rounds=0

  # C1: one per-TU summary per source.  This is required by C2 but is kept
  # only in KEEP_INTERMEDIATES mode.
  for src in "${sources[@]}"; do
    stem="$(source_stem "$project" "$src")"
    out_txt="$c1_text/${stem}.txt"
    if [[ "$SINGLE_TU_MODE" == "1" ]]; then
      src_summary_dir="$source_only_summary_tmp_root/${stem}"
    else
      src_summary_dir="$c1_summary_tmp_root/${stem}"
    fi
    rm -rf "$src_summary_dir"
    mkdir -p "$src_summary_dir"
    explicit_summary="$src_summary_dir/${stem}.cast-summary.json"
    explicit_report="$src_summary_dir/${stem}.cast-report.json"
    explicit_paper_facts="$src_summary_dir/${stem}.cast-paper-facts.json"
    explicit_runtime_metrics="$src_summary_dir/${stem}.cast-runtime.json"
    explicit_prevalence_metrics="$prevalence_metrics_dir/${stem}.cast-prevalence.json"
    if [[ "$SINGLE_TU_MODE" == "1" ]]; then
      cmd=("$TOOL" --summary-json "$explicit_summary")
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        cmd+=(--summary-report-json "$explicit_report")
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        cmd+=(--paper-facts-json "$explicit_paper_facts"
              --paper-facts-project-root "$metrics_project_root")
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        cmd+=(--runtime-timing-json "$explicit_runtime_metrics"
              --runtime-timing-project-id "$safe"
              --runtime-timing-stage c1)
      fi
    else
      cmd=("$TOOL" --summary-dir "$src_summary_dir")
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        cmd+=(--summary-report-dir "$src_summary_dir")
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        cmd+=(--paper-facts-dir "$src_summary_dir"
              --paper-facts-project-root "$metrics_project_root")
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        cmd+=(--runtime-timing-dir "$src_summary_dir"
              --runtime-timing-project-id "$safe"
              --runtime-timing-stage c1)
      fi
    fi
    if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
      cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-c1")
    fi
    if [[ "$PREVALENCE_METRICS" == "1" ]]; then
      cmd+=(--prevalence-metrics-json "$explicit_prevalence_metrics"
            --prevalence-metrics-project-root "$metrics_project_root"
            --prevalence-metrics-project-id "$name")
      if [[ "$PREVALENCE_INCLUDE_RETURN_TYPES" == "1" ]]; then
        cmd+=(--prevalence-include-return-types)
      fi
    fi
    if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
      cmd+=(--analysis-progress --analysis-progress-interval "$ANALYSIS_PROGRESS_INTERVAL_SECONDS")
      if [[ "$ANALYSIS_PROGRESS_VERBOSE" == "1" ]]; then
        cmd+=(--analysis-progress-verbose)
      fi
    fi
    if [[ "$NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK" == "1" ]]; then
      cmd+=(--named-call-pointer-use-membership-cross-check)
    fi
    cmd+=(-p "$compdb_dir" "$src")
    echo "    C1 ${src##*/}"
    printf 'C1: ' >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
    if [[ "$SINGLE_TU_MODE" == "1" ]]; then
      printf 'C1 expected summary JSON: %q\n' "$explicit_summary" >>"$log_file"
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        printf 'C1 expected report JSON: %q\n' "$explicit_report" >>"$log_file"
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        printf 'C1 expected paper-facts JSON: %q\n' "$explicit_paper_facts" >>"$log_file"
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        printf 'C1 expected runtime timing JSON: %q\n' "$explicit_runtime_metrics" >>"$log_file"
      fi
    fi
    if [[ "$PREVALENCE_METRICS" == "1" ]]; then
      printf 'C1 expected prevalence metrics JSON: %q\n' \
        "$explicit_prevalence_metrics" >>"$log_file"
    fi
    c1_analysis_succeeded=0
    c1_tool_exit_code=0
    PARTIAL_PUBLISHED_ARTIFACTS=()
    if RUN_PROGRESS_LABEL="C1 ${src##*/}" RUN_SEPARATE_STDERR="$ANALYSIS_PROGRESS" \
        run_and_capture "$out_txt" "${cmd[@]}"; then
      c1_analysis_succeeded=1
      if [[ "$SINGLE_TU_MODE" == "1" ]]; then
        if ! publish_explicit_summary "$explicit_summary" "$c1_summaries" "$stem" "$log_file" "C1 ${src##*/}"; then
          echo "    C1 summary publish failed: $src"
          project_failed=1
        fi
      elif ! publish_source_summaries "$src_summary_dir" "$c1_summaries" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 summary publish failed: $src"
        project_failed=1
      fi
    else
      c1_tool_exit_code=$?
      echo "    C1 failed: $src (exit $c1_tool_exit_code)"
      append_file_to_log "$log_file" "C1 failure output for ${src##*/}" "$out_txt"
      if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
        append_file_to_log "$log_file" "C1 stderr/progress for ${src##*/}" "${out_txt}.stderr.txt"
      fi
      project_failed=1

      # A nonzero frontend/tool exit does not necessarily mean the AST consumer
      # skipped its end-of-TU writes. Publish any sidecars that exist, but only
      # under partial-* so C2 never consumes a potentially incomplete summary.
      if [[ "$PUBLISH_PARTIAL_C1_SIDECARS" == "1" ]]; then
        if [[ "$SINGLE_TU_MODE" == "1" ]]; then
          publish_partial_file_if_present "$explicit_summary" "$partial_summaries" "$stem" ".cast-summary.json" "summary" "$log_file" "C1 ${src##*/}" || true
          if [[ "$SUMMARY_REPORTS" == "1" ]]; then
            publish_partial_file_if_present "$explicit_report" "$partial_reports" "$stem" ".cast-report.json" "report" "$log_file" "C1 ${src##*/}" || true
          fi
          if [[ "$PAPER_FACTS" == "1" ]]; then
            publish_partial_file_if_present "$explicit_paper_facts" "$partial_paper_facts" "$stem" ".cast-paper-facts.json" "paper-facts" "$log_file" "C1 ${src##*/}" || true
          fi
          if [[ "$RUNTIME_METRICS" == "1" ]]; then
            publish_partial_file_if_present "$explicit_runtime_metrics" "$partial_runtime_metrics" "$stem" ".cast-runtime.json" "runtime-timing" "$log_file" "C1 ${src##*/}" || true
          fi
        else
          publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-summary.json" "$partial_summaries" "$stem" ".cast-summary.json" "summary" "$log_file" "C1 ${src##*/}" || true
          if [[ "$SUMMARY_REPORTS" == "1" ]]; then
            publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-report.json" "$partial_reports" "$stem" ".cast-report.json" "report" "$log_file" "C1 ${src##*/}" || true
          fi
          if [[ "$PAPER_FACTS" == "1" ]]; then
            publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-paper-facts.json" "$partial_paper_facts" "$stem" ".cast-paper-facts.json" "paper-facts" "$log_file" "C1 ${src##*/}" || true
          fi
          if [[ "$RUNTIME_METRICS" == "1" ]]; then
            publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-runtime.json" "$partial_runtime_metrics" "$stem" ".cast-runtime.json" "runtime-timing" "$log_file" "C1 ${src##*/}" || true
          fi
        fi
        write_partial_c1_status_manifest \
          "$partial_status/${stem}.cast-analysis-status.json" \
          "$src" "$stem" "$c1_tool_exit_code" "${stem}.txt" \
          "$(basename "$0")" "${PARTIAL_PUBLISHED_ARTIFACTS[@]}"
        {
          echo
          echo "===== C1 partial sidecar publication for ${src##*/} ====="
          echo "tool exit code: $c1_tool_exit_code"
          echo "C2 eligible: false"
          if [[ ${#PARTIAL_PUBLISHED_ARTIFACTS[@]} -eq 0 ]]; then
            echo "no requested C1 sidecars were emitted before failure"
          else
            printf 'published: %s\n' "${PARTIAL_PUBLISHED_ARTIFACTS[@]}"
          fi
          echo "status manifest: $partial_status/${stem}.cast-analysis-status.json"
        } >>"$log_file"
      fi
    fi
    if [[ "$c1_analysis_succeeded" == "1" && "$SUMMARY_REPORTS" == "1" ]]; then
      if [[ "$SINGLE_TU_MODE" == "1" ]]; then
        if ! publish_explicit_report "$explicit_report" "$c1_reports" "$stem" "$log_file" "C1 ${src##*/}"; then
          echo "    C1 report publish failed: $src"
          project_failed=1
        fi
      elif ! publish_source_reports "$src_summary_dir" "$c1_reports" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 report publish failed: $src"
        project_failed=1
      fi
    fi
    if [[ "$c1_analysis_succeeded" == "1" && "$PAPER_FACTS" == "1" ]]; then
      if [[ "$SINGLE_TU_MODE" == "1" ]]; then
        if ! publish_explicit_paper_facts "$explicit_paper_facts" "$c1_paper_facts" "$stem" "$log_file" "C1 ${src##*/}"; then
          echo "    C1 paper-facts publish failed: $src"
          project_failed=1
        fi
      elif ! publish_source_paper_facts "$src_summary_dir" "$c1_paper_facts" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 paper-facts publish failed: $src"
        project_failed=1
      fi
    fi
    if [[ "$c1_analysis_succeeded" == "1" && "$RUNTIME_METRICS" == "1" ]]; then
      if [[ "$SINGLE_TU_MODE" == "1" ]]; then
        if ! publish_explicit_runtime_metrics "$explicit_runtime_metrics" "$c1_runtime_metrics" "$stem" "$log_file" "C1 ${src##*/}"; then
          echo "    C1 runtime timing publish failed: $src"
          project_failed=1
        fi
      elif ! publish_source_runtime_metrics "$src_summary_dir" "$c1_runtime_metrics" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 runtime timing publish failed: $src"
        project_failed=1
      fi
    fi
    if [[ "$PREVALENCE_METRICS" == "1" && ! -f "$explicit_prevalence_metrics" ]]; then
      echo "    C1 prevalence metrics sidecar missing: $src"
      {
        echo
        echo "===== C1 prevalence metrics publish warning ====="
        echo "expected sidecar: $explicit_prevalence_metrics"
        echo "runner output root (absolute): $OUTPUT_ROOT"
      } >>"$log_file"
      project_failed=1
    fi
  done

  if [[ "$SINGLE_TU_MODE" == "1" ]]; then
    echo "    source-only mode: skipping C2 index construction and summary-import rounds"
  else
    # C2: build a project-level index from C1 summaries.  The index itself is
    # part of the final output and is refined after each import round below.
    c2_cmd=("$PYTHON" "$BUILD_INDEX" "$c1_summaries" --project-kind "$PROJECT_KIND" -o "$index_file")
  echo "    C2 build index from raw summaries"
  printf 'C2: ' >>"$log_file"; printf '%q ' "${c2_cmd[@]}" >>"$log_file"; echo >>"$log_file"
  if ! RUN_PROGRESS_LABEL="C2 build index" run_and_capture "$c2_txt" "${c2_cmd[@]}"; then
    echo "    C2 failed: build_summary_index.py"
    append_file_to_log "$log_file" "C2 failure output" "$c2_txt"
    project_failed=1
  elif [[ "$KEEP_INTERMEDIATES" != "1" ]]; then
    append_file_to_log "$log_file" "C2 index build output" "$c2_txt"
  fi

  # Import/index refinement rounds.  SUMMARY_IMPORT_ROUNDS=2 implements the
  # C4G workflow:
  #   C1 summaries -> index0 -> import round 1 -> index1 -> import round 2
  # Increase SUMMARY_IMPORT_ROUNDS when improvements must travel through
  # additional caller layers or longer dependency chains.
  if [[ -f "$index_for_next_round" ]]; then
    for ((round = 1; round <= SUMMARY_IMPORT_ROUNDS; round++)); do
      round_tag="$(printf 'import-round-%02d' "$round")"
      round_text="$rounds_root/${round_tag}-text"
      round_summaries="$rounds_root/${round_tag}-summaries"
      round_reports="$rounds_root/${round_tag}-reports"
      round_paper_facts="$rounds_root/${round_tag}-paper-facts"
      round_runtime_metrics="$rounds_root/${round_tag}-runtime-metrics"
      # Keep the index consumed by each import round at the project output
      # root, even when the round's text/summaries are temporary.  The index
      # contains summaryFile entries relative to the project output root; if the
      # input index is copied under .tmp-intermediates, SummaryIndex resolves
      # paths like .tmp-intermediates/import-round-01-summaries/... relative to
      # .tmp-intermediates and accidentally doubles the prefix.
      round_index_input="$project_out/${round_tag}-index-input.json"
      round_index_log="$rounds_root/${round_tag}-build-index.txt"
      mkdir -p "$round_text" "$round_summaries"
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        mkdir -p "$round_reports"
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        mkdir -p "$round_paper_facts"
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        mkdir -p "$round_runtime_metrics"
      fi
      cp "$index_for_next_round" "$round_index_input"
      last_import_index="$round_index_input"

      echo "    import round $round/$SUMMARY_IMPORT_ROUNDS"
      for src in "${sources[@]}"; do
        stem="$(source_stem "$project" "$src")"
        out_txt="$round_text/${stem}.txt"
        round_summary_tmp_root="$rounds_root/${round_tag}-summary-tmp"
        src_summary_dir="$round_summary_tmp_root/${stem}"
        rm -rf "$src_summary_dir"
        mkdir -p "$src_summary_dir"
        cmd=("$TOOL" --enable-summary-import --summary-index "$round_index_input" --summary-dir "$src_summary_dir")
        if [[ "$SUMMARY_REPORTS" == "1" ]]; then
          cmd+=(--summary-report-dir "$src_summary_dir")
        fi
        if [[ "$PAPER_FACTS" == "1" ]]; then
          cmd+=(--paper-facts-dir "$src_summary_dir"
                --paper-facts-project-root "$metrics_project_root")
        fi
        if [[ "$RUNTIME_METRICS" == "1" ]]; then
          cmd+=(--runtime-timing-dir "$src_summary_dir"
                --runtime-timing-project-id "$safe"
                --runtime-timing-stage "$round_tag")
        fi
        if [[ "$SUMMARY_IMPORT_DEBUG" == "1" ]]; then
          cmd+=(--summary-import-debug)
        fi
        if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
          cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-${round_tag}")
        fi
        if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
          cmd+=(--analysis-progress --analysis-progress-interval "$ANALYSIS_PROGRESS_INTERVAL_SECONDS")
          if [[ "$ANALYSIS_PROGRESS_VERBOSE" == "1" ]]; then
            cmd+=(--analysis-progress-verbose)
          fi
        fi
        if [[ "$NAMED_CALL_POINTER_USE_MEMBERSHIP_CROSS_CHECK" == "1" ]]; then
          cmd+=(--named-call-pointer-use-membership-cross-check)
        fi
        cmd+=(-p "$compdb_dir" "$src")
        echo "      ${round_tag} ${src##*/}"
        printf '%s: ' "$round_tag" >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
        round_analysis_succeeded=0
        if ! RUN_PROGRESS_LABEL="${round_tag} ${src##*/}" RUN_SEPARATE_STDERR="$ANALYSIS_PROGRESS" \
            run_and_capture "$out_txt" "${cmd[@]}"; then
          echo "    ${round_tag} failed: $src"
          append_file_to_log "$log_file" "${round_tag} failure output for ${src##*/}" "$out_txt"
          if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
            append_file_to_log "$log_file" "${round_tag} stderr/progress for ${src##*/}" "${out_txt}.stderr.txt"
          fi
          project_failed=1
        else
          round_analysis_succeeded=1
          if ! publish_source_summaries "$src_summary_dir" "$round_summaries" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
            echo "    ${round_tag} summary publish failed: $src"
            project_failed=1
          fi
        fi
        if [[ "$round_analysis_succeeded" == "1" && "$SUMMARY_REPORTS" == "1" ]] && ! publish_source_reports "$src_summary_dir" "$round_reports" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
          echo "    ${round_tag} report publish failed: $src"
          project_failed=1
        fi
        if [[ "$round_analysis_succeeded" == "1" && "$PAPER_FACTS" == "1" ]] && ! publish_source_paper_facts "$src_summary_dir" "$round_paper_facts" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
          echo "    ${round_tag} paper-facts publish failed: $src"
          project_failed=1
        fi
        if [[ "$round_analysis_succeeded" == "1" && "$RUNTIME_METRICS" == "1" ]] && ! publish_source_runtime_metrics "$src_summary_dir" "$round_runtime_metrics" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
          echo "    ${round_tag} runtime timing publish failed: $src"
          project_failed=1
        fi
      done

      latest_text="$round_text"
      latest_summaries="$round_summaries"
      latest_reports="$round_reports"
      latest_paper_facts="$round_paper_facts"
      latest_runtime_metrics="$round_runtime_metrics"
      completed_import_rounds="$round"

      # Rebuild the project index from the just-produced import-aware summaries.
      # The next round will consume this refreshed index, which is where C4G's
      # materialized solvedParamSummaries become visible to every TU.
      if compgen -G "$round_summaries/*.cast-summary.json" >/dev/null; then
        rebuild_cmd=("$PYTHON" "$BUILD_INDEX" "$round_summaries" --project-kind "$PROJECT_KIND" -o "$index_file")
        echo "    ${round_tag} rebuild index from import-aware summaries"
        printf 'rebuild-index-after-%s: ' "$round_tag" >>"$log_file"
        printf '%q ' "${rebuild_cmd[@]}" >>"$log_file"; echo >>"$log_file"
        if ! RUN_PROGRESS_LABEL="${round_tag} rebuild index" run_and_capture "$round_index_log" "${rebuild_cmd[@]}"; then
          echo "    ${round_tag} index rebuild failed: build_summary_index.py"
          append_file_to_log "$log_file" "${round_tag} index rebuild output" "$round_index_log"
          project_failed=1
          break
        elif [[ "$KEEP_INTERMEDIATES" != "1" ]]; then
          append_file_to_log "$log_file" "${round_tag} index rebuild output" "$round_index_log"
        fi
        index_for_next_round="$index_file"
      else
        echo "    ${round_tag} index rebuild skipped: no summaries emitted"
        project_failed=1
        break
      fi
    done
  else
    echo "    import rounds skipped: missing index $index_for_next_round"
    project_failed=1
  fi

  fi  # SINGLE_TU_MODE

  # Publish the latest outputs. In SINGLE_TU_MODE these are the independent
  # C1 outputs; otherwise they are the final import-aware outputs (or C1 when
  # SUMMARY_IMPORT_ROUNDS=0).
  for src in "${sources[@]}"; do
    stem="$(source_stem "$project" "$src")"
    if [[ -f "$latest_text/${stem}.txt" ]]; then
      cp "$latest_text/${stem}.txt" "$final_text/${stem}.txt"
    fi
    if [[ "$ANALYSIS_PROGRESS" == "1" &&
          -f "$latest_text/${stem}.txt.stderr.txt" ]]; then
      cp "$latest_text/${stem}.txt.stderr.txt" \
         "$final_text/${stem}.progress.txt"
    fi
  done

  if compgen -G "$latest_summaries/*.cast-summary.json" >/dev/null; then
    cp "$latest_summaries"/*.cast-summary.json "$final_summaries"/
  fi
  if [[ "$SUMMARY_REPORTS" == "1" ]] &&
      compgen -G "$latest_reports/*.cast-report.json" >/dev/null; then
    cp "$latest_reports"/*.cast-report.json "$final_reports"/
  fi
  if [[ "$PAPER_FACTS" == "1" ]] &&
      compgen -G "$latest_paper_facts/*.cast-paper-facts.json" >/dev/null; then
    cp "$latest_paper_facts"/*.cast-paper-facts.json "$final_paper_facts"/
  fi

  if [[ "$RUNTIME_METRICS" == "1" ]] &&
      compgen -G "$latest_runtime_metrics/*.cast-runtime.json" >/dev/null; then
    cp "$latest_runtime_metrics"/*.cast-runtime.json "$final_runtime_metrics"/
  fi

  if [[ "$RUNTIME_METRICS" == "1" ]] &&
      compgen -G "$final_runtime_metrics/*.cast-runtime.json" >/dev/null; then
    runtime_per_tu_csv="$project_out/runtime-duration-by-tu.csv"
    runtime_per_project_csv="$project_out/runtime-duration-by-project.csv"
    runtime_cmd=("$PYTHON" "$RUNTIME_AGGREGATOR" "$final_runtime_metrics"
                 --per-tu-csv "$runtime_per_tu_csv"
                 --per-project-csv "$runtime_per_project_csv"
                 --latex-out "$project_out/runtime-duration.tex")
    echo "    aggregate final runtime timings"
    printf 'runtime-aggregate: ' >>"$log_file"
    printf '%q ' "${runtime_cmd[@]}" >>"$log_file"
    echo >>"$log_file"
    if ! RUN_PROGRESS_LABEL="runtime timing aggregation" run_and_capture "$project_out/runtime-timing-aggregation.txt" "${runtime_cmd[@]}"; then
      echo "    runtime timing aggregation failed"
      append_file_to_log "$log_file" "runtime timing aggregation output" "$project_out/runtime-timing-aggregation.txt"
      project_failed=1
    elif [[ "$KEEP_INTERMEDIATES" != "1" ]]; then
      append_file_to_log "$log_file" "runtime timing aggregation output" "$project_out/runtime-timing-aggregation.txt"
      rm -f "$project_out/runtime-timing-aggregation.txt"
    fi
  fi

  if [[ -n "$last_import_index" && -f "$last_import_index" ]]; then
    cp "$last_import_index" "$final_import_index"
  fi

  if [[ "$SINGLE_TU_MODE" == "1" ]]; then
    echo "    completed independent source analyses: ${#sources[@]}"
  else
    echo "    completed import rounds: $completed_import_rounds"
  fi
  if [[ "$ANALYSIS_PROGRESS" == "1" ]]; then
    echo "    analyzer progress logs: $final_text/*.progress.txt"
  fi

  if [[ "$RUNTIME_METRICS" == "1" ]]; then
    echo "    runtime timings: $project_out/runtime-duration-by-project.csv"
  fi

  if [[ -n "${source_only_summary_tmp_root:-}" ]]; then
    rm -rf "$source_only_summary_tmp_root"
  fi

  if [[ "$KEEP_INTERMEDIATES" != "1" && -n "${tmp_root:-}" ]]; then
    rm -rf "$tmp_root"
  fi

  if [[ $project_failed -eq 0 ]]; then
    ((passed++))
    echo "    PASS"
  else
    ((failed++))
    echo "    FAIL"
    if [[ "$STOP_ON_FAIL" == "1" ]]; then
      echo "Stopping on first failed project because STOP_ON_FAIL=1"
      break
    fi
  fi
  echo
done

if [[ "$RUNTIME_METRICS" == "1" ]]; then
  mapfile -t final_runtime_dirs < <(find "$OUTPUT_ROOT" -mindepth 2 -maxdepth 2 -type d -name runtime-metrics | sort)
  if [[ ${#final_runtime_dirs[@]} -gt 0 ]]; then
    global_runtime_tu_csv="$OUTPUT_ROOT/runtime-duration-by-tu.csv"
    global_runtime_project_csv="$OUTPUT_ROOT/runtime-duration-by-project.csv"
    global_runtime_cmd=("$PYTHON" "$RUNTIME_AGGREGATOR" "${final_runtime_dirs[@]}"
                        --per-tu-csv "$global_runtime_tu_csv"
                        --per-project-csv "$global_runtime_project_csv"
                        --latex-out "$OUTPUT_ROOT/runtime-duration.tex")
    echo "Aggregating final runtime timings across projects"
    if ! "${global_runtime_cmd[@]}"; then
      echo "warning: global runtime timing aggregation failed" >&2
    fi
  fi
fi

echo "Done."
echo "  passed projects:  $passed"
echo "  failed projects:  $failed"
echo "  skipped projects: $skipped"
echo "  output root:      $OUTPUT_ROOT"

if [[ $failed -ne 0 ]]; then
  exit 1
fi
exit 0

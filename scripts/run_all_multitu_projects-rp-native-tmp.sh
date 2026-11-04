#!/usr/bin/env bash
set -u

# Run multi-TU projects through the summary/export + iterative import/index workflow.
# Source files can be discovered from compile_commands.json, so projects may
# have nested source trees rather than only top-level *.c files.
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
#   NORMALIZE_PATHS=0
#   PROJECT_KIND=real
#   STOP_ON_FAIL=0
#   KEEP_INTERMEDIATES=0
#   SUMMARY_REPORTS=0                  # 1: emit verbose *.cast-report.json sidecars
#   PAPER_FACTS=0                      # 1: emit compact *.cast-paper-facts.json sidecars
#   RUNTIME_METRICS=0                  # 1: emit compact *.cast-runtime.json sidecars and CSV timing tables
#   RESOURCE_DIR=<clang-resource-dir>  # injected into all compile-database TU parses
#   NATIVE_ARTIFACT_TMPDIR=/tmp          # native staging for analyzer JSON sidecars
#   CLANG_BIN=clang                    # executable used when RESOURCE_DIR is not set
#   RESOURCE_DIR=<clang-resource-dir>  # injected for all compile-database analyzer invocations
#   PUBLISH_PARTIAL_C1_SIDECARS=1  # retain C1 JSON/timing sidecars after nonzero analyzer exits
#
# JSON summary collision avoidance:
#   Each source file is analyzed with its own temporary --summary-dir and the
#   emitted *.cast-summary.json is copied into the round summary directory using
#   the same collision-safe relative-path stem as the text output.  For example:
#     src/a.c   -> src_a.c.cast-summary.json
#     tests/a.c -> tests_a.c.cast-summary.json
#
# Default output layout for each project:
#   <OUTPUT_ROOT>/<project-name>/
#     <source>.txt                       # final import-aware text output
#     summaries/*.cast-summary.json      # compact C2/import link summaries
#     reports/*.cast-report.json         # optional verbose diagnostics (SUMMARY_REPORTS=1)
#     paper-facts/*.cast-paper-facts.json # optional compact paper facts (PAPER_FACTS=1)
#     runtime-metrics/*.cast-runtime.json # optional runtime timing sidecars (RUNTIME_METRICS=1)
#     runtime-duration-by-tu.csv      # final-stage timing table by translation unit
#     runtime-duration-by-project.csv # final-stage timing table by project
#     project-summary-index.json         # index rebuilt from final summaries
#     project-summary-index-used-for-final-import.json
#                                       # index consumed by the final import round
#     import-round-01-index-input.json  # round input indexes are kept at
#     import-round-02-index-input.json  # project root so relative summaryFile
#     ...                               # paths resolve from the same directory
#     partial-summaries/*.cast-summary.json       # failed C1 sidecars; never consumed by C2
#     partial-reports/*.cast-report.json          # failed C1 report sidecars
#     partial-paper-facts/*.cast-paper-facts.json # failed C1 paper-facts sidecars
#     partial-runtime-metrics/*.cast-runtime.json # failed C1 timing sidecars
#     partial-status/*.cast-analysis-status.json  # failed C1 status manifests
#     commands.log
#
# With KEEP_INTERMEDIATES=1, debug artifacts are also preserved:
#   <OUTPUT_ROOT>/<project-name>/
#     c1-text/*.txt
#     c1-summaries/*.cast-summary.json
#     c2-build-index.txt
#     import-round-01-text/*.txt
#     import-round-01-summaries/*.cast-summary.json
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
SUMMARY_IMPORT_DEBUG="${SUMMARY_IMPORT_DEBUG:-0}"
SUMMARY_IMPORT_ROUNDS="${SUMMARY_IMPORT_ROUNDS:-1}"
NORMALIZE_PATHS="${NORMALIZE_PATHS:-0}"
PROJECT_KIND="${PROJECT_KIND:-real}"
STOP_ON_FAIL="${STOP_ON_FAIL:-0}"
KEEP_INTERMEDIATES="${KEEP_INTERMEDIATES:-0}"
SUMMARY_REPORTS="${SUMMARY_REPORTS:-0}"
PAPER_FACTS="${PAPER_FACTS:-0}"
RUNTIME_METRICS="${RUNTIME_METRICS:-0}"

# C6G5W1G_NATIVE_SIDECAR_STAGING
# Analyzer JSON sidecars are first written to a native Linux filesystem.
# This avoids filesystem/encoding/atomic-rename problems when OUTPUT_ROOT is
# under a mounted Windows drive such as /mnt/d. The runner then copies each
# completed artifact into the ordinary durable project output tree.
NATIVE_ARTIFACT_TMPDIR="${NATIVE_ARTIFACT_TMPDIR:-${TMPDIR:-/tmp}}"

if [[ ! -d "$NATIVE_ARTIFACT_TMPDIR" || ! -w "$NATIVE_ARTIFACT_TMPDIR" ]]; then
  echo "error: NATIVE_ARTIFACT_TMPDIR is not a writable directory: $NATIVE_ARTIFACT_TMPDIR" >&2
  exit 1
fi

declare -a NATIVE_ARTIFACT_TMP_ROOTS=()

cleanup_native_artifact_staging() {
  local native_dir
  if [[ "${KEEP_INTERMEDIATES:-0}" == "1" ]]; then
    return 0
  fi
  for native_dir in "${NATIVE_ARTIFACT_TMP_ROOTS[@]}"; do
    [[ -n "$native_dir" ]] && rm -rf "$native_dir"
  done
}
trap cleanup_native_artifact_staging EXIT


# Clang builtin headers such as stddef.h live below the resource directory.
# Compile databases usually omit -resource-dir because the compiler driver
# supplies it implicitly; LibTooling needs it explicitly when this analyzer is
# run with -p <compile-commands-dir>. Set RESOURCE_DIR to override discovery
# or CLANG_BIN to select the matching clang executable.
CLANG_BIN="${CLANG_BIN:-clang}"
RESOURCE_DIR="${RESOURCE_DIR:-}"

if [[ -z "$RESOURCE_DIR" ]]; then
  if ! command -v "$CLANG_BIN" >/dev/null 2>&1; then
    echo "error: CLANG_BIN is not executable: $CLANG_BIN" >&2
    echo "       set CLANG_BIN or RESOURCE_DIR explicitly." >&2
    exit 1
  fi
  if ! RESOURCE_DIR="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)"; then
    echo "error: could not determine Clang resource directory using: $CLANG_BIN -print-resource-dir" >&2
    echo "       set RESOURCE_DIR explicitly." >&2
    exit 1
  fi
fi

if [[ ! -f "$RESOURCE_DIR/include/stddef.h" ]]; then
  echo "error: invalid Clang resource directory: $RESOURCE_DIR" >&2
  echo "       expected builtin header: $RESOURCE_DIR/include/stddef.h" >&2
  echo "       set RESOURCE_DIR to the resource directory of the clang matching cast-history-tool." >&2
  exit 1
fi

# One array expansion is reused by C1, local-index-overlay, and every import
# replay. --extra-arg-before keeps the injected compiler option ahead of the
# per-TU compile-command arguments while preserving each database entry.
CLANG_RESOURCE_DIR_TOOL_ARGS=(
  "--extra-arg-before=-resource-dir=$RESOURCE_DIR"
)

PUBLISH_PARTIAL_C1_SIDECARS="${PUBLISH_PARTIAL_C1_SIDECARS:-1}"
USE_COMPILE_COMMANDS_SOURCES="${USE_COMPILE_COMMANDS_SOURCES:-1}"
RECURSIVE_SOURCE_SCAN="${RECURSIVE_SOURCE_SCAN:-0}"
SOURCE_EXTENSIONS="${SOURCE_EXTENSIONS:-.c .cc .cpp}"
SOURCE_FILTER_REGEX="${SOURCE_FILTER_REGEX:-}"
SOURCE_EXCLUDE_REGEX="${SOURCE_EXCLUDE_REGEX:-}"
REQUIRE_SOURCE_EXISTS="${REQUIRE_SOURCE_EXISTS:-1}"
export SOURCE_EXTENSIONS SOURCE_FILTER_REGEX SOURCE_EXCLUDE_REGEX REQUIRE_SOURCE_EXISTS

if ! [[ "$SUMMARY_IMPORT_ROUNDS" =~ ^[0-9]+$ ]]; then
  echo "error: SUMMARY_IMPORT_ROUNDS must be a non-negative integer: $SUMMARY_IMPORT_ROUNDS" >&2
  exit 1
fi

for bool_name in SUMMARY_REPORTS PAPER_FACTS RUNTIME_METRICS PUBLISH_PARTIAL_C1_SIDECARS; do
  bool_value="${!bool_name}"
  if [[ "$bool_value" != "0" && "$bool_value" != "1" ]]; then
    echo "error: $bool_name must be 0 or 1: $bool_value" >&2
    exit 1
  fi
done

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

if [[ ! -f "$BUILD_INDEX" ]]; then
  echo "error: C2 index builder not found: $BUILD_INDEX" >&2
  echo "hint: copy scripts/build_summary_index.py into the repo or set BUILD_INDEX=/path/to/build_summary_index.py" >&2
  exit 1
fi

if [[ "$RUNTIME_METRICS" == "1" && ! -f "$RUNTIME_AGGREGATOR" ]]; then
  echo "error: runtime timing aggregator not found: $RUNTIME_AGGREGATOR" >&2
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
OUTPUT_ROOT_ABS="$(cd "$OUTPUT_ROOT" && pwd -P)"
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

run_and_capture() {
  local out_file="$1"
  shift
  local tmp="${out_file}.tmp"
  mkdir -p "$(dirname "$out_file")"
  {
    echo "Command:"
    printf '%q ' "$@"
    echo
    echo
    echo "Output:"
    "$@"
  } >"$tmp" 2>&1
  local status=$?
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


# C6g5t: Import rounds are only useful when C1 discovered at least one
# cross-TU call edge.  A single-TU project still gets a C2 index for auditing,
# but replaying it through import rounds merely re-runs an already quiescent
# local SCC solver and rebuilds the same index.
project_index_has_importable_edges() {
  local index_file="$1"
  "$PYTHON" - "$index_file" <<'PY_INDEX'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
try:
    data = json.loads(path.read_text(encoding="utf-8"))
except Exception:
    # Fail closed: a malformed/legacy index must retain import rounds rather
    # than silently omitting potentially cross-TU work.
    raise SystemExit(0)

edges = data.get("summaryDependencyEdges")
if not isinstance(edges, list):
    raise SystemExit(0)
# An index that explicitly records every edge as local can skip C2 replay.
# Missing topology metadata remains conservative.
if any(not isinstance(e, dict) or "crossTUCandidate" not in e for e in edges):
    raise SystemExit(0)
raise SystemExit(0 if any(bool(e.get("crossTUCandidate")) for e in edges) else 1)
PY_INDEX
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
    mapfile -t summaries < <(find "$src_summary_dir" -maxdepth 1 -type f -name '*.cast-summary.json' | sort)
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

# Publish a C6g5e verbose report sidecar using the same collision-safe source
# stem as link summaries.  Reports are never supplied to the C2 index builder.
publish_source_reports() {
  local src_report_dir="$1"
  local dest_report_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_report_dir"
  local reports=()
  if [[ -d "$src_report_dir" ]]; then
    mapfile -t reports < <(find "$src_report_dir" -maxdepth 1 -type f -name '*.cast-report.json' | sort)
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
    {
      echo
      echo "===== ${label} report publish warning ====="
      echo "multiple reports emitted for one source; copied first to: $dest"
      echo "extra reports copied with __extra_N suffixes"
      printf 'source report dir: %s\n' "$src_report_dir"
    } >>"$log_file"
  fi
  return 0
}


# Publish a compact C6g5f paper-facts sidecar using the same collision-safe
# source stem.  These files are never passed to C2/index construction.
publish_source_paper_facts() {
  local src_dir="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_dir"
  local facts=()
  if [[ -d "$src_dir" ]]; then
    mapfile -t facts < <(find "$src_dir" -maxdepth 1 -type f -name '*.cast-paper-facts.json' | sort)
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
    {
      echo
      echo "===== ${label} paper-facts publish warning ====="
      echo "multiple paper-facts sidecars emitted for one source; copied first to: $dest"
      echo "extra sidecars copied with __extra_N suffixes"
      printf 'source paper-facts dir: %s\n' "$src_dir"
    } >>"$log_file"
  fi
  return 0
}


# Publish a compact C6g5g runtime timing sidecar using the same collision-safe
# source stem. These files are never passed to C2/index construction.
publish_source_runtime_metrics() {
  local src_dir="$1"
  local dest_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  mkdir -p "$dest_dir"
  local timings=()
  if [[ -d "$src_dir" ]]; then
    mapfile -t timings < <(find "$src_dir" -maxdepth 1 -type f -name '*.cast-runtime.json' | sort)
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
    {
      echo
      echo "===== ${label} runtime-timing publish warning ====="
      echo "multiple runtime timing sidecars emitted for one source; copied first to: $dest"
      echo "extra sidecars copied with __extra_N suffixes"
      printf 'source runtime timing dir: %s\n' "$src_dir"
    } >>"$log_file"
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

# C6G5W1G_NATIVE_SIDECAR_STAGING
# Redefine normal sidecar publishers to discover the single artifact emitted
# under a private native per-TU directory. The analyzer derives filenames from
# the absolute source path, so the runner must not assume a basename-derived
# source filename here.
publish_rp_discovered_sidecar() {
  local source_dir="$1"
  local destination_dir="$2"
  local stem="$3"
  local suffix="$4"
  local artifact_kind="$5"
  local log_file="$6"
  local label="$7"

  local candidates=()
  if [[ -d "$source_dir" ]]; then
    mapfile -t candidates < <(find "$source_dir" -type f -name "*${suffix}" -print | sort)
  fi

  if [[ ${#candidates[@]} -ne 1 ]]; then
    {
      echo
      echo "===== ${label} ${artifact_kind} publish warning ====="
      echo "native staging directory: $source_dir"
      echo "expected exactly one *${suffix}; found: ${#candidates[@]}"
      if [[ ${#candidates[@]} -gt 0 ]]; then
        printf 'candidate: %s\n' "${candidates[@]}"
      fi
    } >>"$log_file"
    return 1
  fi

  mkdir -p "$destination_dir"
  local destination="$destination_dir/${stem}${suffix}"
  if ! cp "${candidates[0]}" "$destination"; then
    {
      echo
      echo "===== ${label} ${artifact_kind} publish warning ====="
      echo "copy failed"
      echo "source: ${candidates[0]}"
      echo "destination: $destination"
    } >>"$log_file"
    return 1
  fi
  return 0
}

publish_source_summaries() {
  publish_rp_discovered_sidecar "$1" "$2" "$3" ".cast-summary.json" "summary" "$4" "$5"
}

publish_source_reports() {
  publish_rp_discovered_sidecar "$1" "$2" "$3" ".cast-report.json" "report" "$4" "$5"
}

publish_source_paper_facts() {
  publish_rp_discovered_sidecar "$1" "$2" "$3" ".cast-paper-facts.json" "paper-facts" "$4" "$5"
}

publish_source_runtime_metrics() {
  publish_rp_discovered_sidecar "$1" "$2" "$3" ".cast-runtime.json" "runtime-timing" "$4" "$5"
}

publish_native_prevalence_metrics() {
  local source_file="$1"
  local destination_dir="$2"
  local stem="$3"
  local log_file="$4"
  local label="$5"

  if [[ ! -f "$source_file" ]]; then
    {
      echo
      echo "===== ${label} prevalence publish warning ====="
      echo "native prevalence sidecar missing: $source_file"
    } >>"$log_file"
    return 1
  fi

  mkdir -p "$destination_dir"
  local destination="$destination_dir/${stem}.cast-prevalence.json"
  if ! cp "$source_file" "$destination"; then
    {
      echo
      echo "===== ${label} prevalence publish warning ====="
      echo "copy failed"
      echo "source: $source_file"
      echo "destination: $destination"
    } >>"$log_file"
    return 1
  fi
  return 0
}

mapfile -t projects < <(collect_projects)

if [[ ${#projects[@]} -eq 0 ]]; then
  echo "warning: no multi-TU projects found under $PROJECT_ROOT" >&2
  exit 0
fi

passed=0
failed=0
skipped=0

echo "Running ${#projects[@]} multi-TU project(s)"
echo "  tool:                 $TOOL"
echo "  tool root:            $TOOL_ROOT_ABS"
echo "  project root:         $PROJECT_ROOT"
echo "  project sets:         $PROJECT_SETS"
if [[ -n "${PROJECT_PATH:-}" ]]; then
  echo "  project path:         $PROJECT_PATH"
fi
echo "  output root:          $OUTPUT_ROOT"
echo "  C2 index builder:     $BUILD_INDEX"
echo "  project kind:         $PROJECT_KIND"
echo "  summary import debug: $SUMMARY_IMPORT_DEBUG"
echo "  import rounds:        $SUMMARY_IMPORT_ROUNDS"
echo "  normalized paths:     $NORMALIZE_PATHS"
echo "  keep intermediates:   $KEEP_INTERMEDIATES"
echo "  verbose reports:       $SUMMARY_REPORTS"
echo "  compact paper facts:   $PAPER_FACTS"
echo "  runtime metrics:       $RUNTIME_METRICS"
echo "  partial C1 sidecars:  $PUBLISH_PARTIAL_C1_SIDECARS"
if [[ "$RUNTIME_METRICS" == "1" ]]; then
  echo "  runtime aggregator:    $RUNTIME_AGGREGATOR"
fi
echo "  source discovery:     $([[ "$USE_COMPILE_COMMANDS_SOURCES" == "1" ]] && echo compile_commands || echo source-scan)"
echo "  recursive fallback:   $RECURSIVE_SOURCE_SCAN"
echo "  source extensions:    $SOURCE_EXTENSIONS"
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
    c1_summary_tmp_root="$project_out/c1-summary-tmp"
    c1_reports="$project_out/c1-reports"
    c1_paper_facts="$project_out/c1-paper-facts"
    c1_runtime_metrics="$project_out/c1-runtime-metrics"
    c1_report_tmp_root="$project_out/c1-report-tmp"
    c2_txt="$project_out/c2-build-index.txt"
    rounds_root="$project_out"
  else
    tmp_root="$project_out/.tmp-intermediates"
    c1_text="$tmp_root/c1-text"
    c1_summaries="$tmp_root/c1-summaries"
    c1_summary_tmp_root="$tmp_root/c1-summary-tmp"
    c1_reports="$tmp_root/c1-reports"
    c1_paper_facts="$tmp_root/c1-paper-facts"
    c1_runtime_metrics="$tmp_root/c1-runtime-metrics"
    c1_report_tmp_root="$tmp_root/c1-report-tmp"
    c2_txt="$tmp_root/c2-build-index.txt"
    rounds_root="$tmp_root"
  fi

  rm -rf "$project_out"
  mkdir -p "$final_text" "$final_summaries" \
           "$c1_text" "$c1_summaries" "$c1_summary_tmp_root" "$rounds_root"
  if [[ "$SUMMARY_REPORTS" == "1" ]]; then
    mkdir -p "$final_reports" "$c1_reports" "$c1_report_tmp_root"
  fi
  if [[ "$PAPER_FACTS" == "1" ]]; then
    mkdir -p "$final_paper_facts" "$c1_paper_facts"
  fi
  if [[ "$RUNTIME_METRICS" == "1" ]]; then
    mkdir -p "$final_runtime_metrics" "$c1_runtime_metrics"
  fi
  : >"$log_file"
  if ! native_artifact_tmp_root="$(
      mktemp -d "${NATIVE_ARTIFACT_TMPDIR%/}/cast-history-rp.XXXXXX"
    )"; then
    echo "error: could not create native artifact staging directory under: $NATIVE_ARTIFACT_TMPDIR" >&2
    exit 1
  fi
  NATIVE_ARTIFACT_TMP_ROOTS+=("$native_artifact_tmp_root")
  printf 'Clang resource directory: %q\n' "$RESOURCE_DIR" >>"$log_file"
  printf 'Native artifact staging root: %q\n' "$native_artifact_tmp_root" >>"$log_file"
  printf 'Clang resource directory: %q\n' "$RESOURCE_DIR" >>"$log_file"

  project_dir="$(project_dir_for_tool "$project")"
  compdb="$(compile_commands_for_project "$project")"
  if [[ -z "$compdb" || ! -f "$compdb" ]]; then
    echo "==> $name: SKIP (no compile_commands.json)"
    ((skipped++))
    continue
  fi

  mapfile -t sources < <(collect_sources "$project")
  if [[ ${#sources[@]} -eq 0 ]]; then
    echo "==> $name: SKIP (no source files)"
    ((skipped++))
    continue
  fi

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
    src_summary_dir="$native_artifact_tmp_root/c1/${stem}/sidecars"
    src_report_dir=""
    rm -rf "$src_summary_dir"
    mkdir -p "$src_summary_dir"
    cmd=("$TOOL" --summary-dir "$src_summary_dir")
    if [[ "$SUMMARY_REPORTS" == "1" ]]; then
      src_report_dir="$native_artifact_tmp_root/c1/${stem}/reports"
      rm -rf "$src_report_dir"
      mkdir -p "$src_report_dir"
      cmd+=(--summary-report-dir "$src_report_dir")
    fi
    if [[ "$PAPER_FACTS" == "1" ]]; then
      cmd+=(--paper-facts-dir "$src_summary_dir")
    fi
    if [[ "$RUNTIME_METRICS" == "1" ]]; then
      cmd+=(--runtime-timing-dir "$src_summary_dir"
            --runtime-timing-project-id "$safe"
            --runtime-timing-stage c1)
    fi
    if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
      cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-c1")
    fi
    cmd+=("${CLANG_RESOURCE_DIR_TOOL_ARGS[@]}")
    cmd+=(-p "$project_dir" "$src")
    echo "    C1 ${src##*/}"
    printf 'C1: ' >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
    c1_tool_exit_code=0
    PARTIAL_PUBLISHED_ARTIFACTS=()
    if run_and_capture "$out_txt" "${cmd[@]}"; then
      if ! publish_source_summaries "$src_summary_dir" "$c1_summaries" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 summary publish failed: $src"
        project_failed=1
      fi
      if [[ "$SUMMARY_REPORTS" == "1" ]] && ! publish_source_reports "$src_report_dir" "$c1_reports" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 report publish failed: $src"
        project_failed=1
      fi
      if [[ "$PAPER_FACTS" == "1" ]] && ! publish_source_paper_facts "$src_summary_dir" "$c1_paper_facts" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 paper-facts publish failed: $src"
        project_failed=1
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]] && ! publish_source_runtime_metrics "$src_summary_dir" "$c1_runtime_metrics" "$stem" "$log_file" "C1 ${src##*/}"; then
        echo "    C1 runtime-timing publish failed: $src"
        project_failed=1
      fi
    else
      c1_tool_exit_code=$?
      echo "    C1 failed: $src (exit $c1_tool_exit_code)"
      append_file_to_log "$log_file" "C1 failure output for ${src##*/}" "$out_txt"
      project_failed=1
      if [[ "$PUBLISH_PARTIAL_C1_SIDECARS" == "1" ]]; then
        publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-summary.json" "$partial_summaries" "$stem" ".cast-summary.json" "summary" "$log_file" "C1 ${src##*/}" || true
        if [[ "$SUMMARY_REPORTS" == "1" ]]; then
          publish_partial_discovered_sidecar_if_present "$src_report_dir" "*.cast-report.json" "$partial_reports" "$stem" ".cast-report.json" "report" "$log_file" "C1 ${src##*/}" || true
        fi
        if [[ "$PAPER_FACTS" == "1" ]]; then
          publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-paper-facts.json" "$partial_paper_facts" "$stem" ".cast-paper-facts.json" "paper-facts" "$log_file" "C1 ${src##*/}" || true
        fi
        if [[ "$RUNTIME_METRICS" == "1" ]]; then
          publish_partial_discovered_sidecar_if_present "$src_summary_dir" "*.cast-runtime.json" "$partial_runtime_metrics" "$stem" ".cast-runtime.json" "runtime-timing" "$log_file" "C1 ${src##*/}" || true
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
  done

  # C2: build a project-level index from C1 summaries.  The index itself is
  # part of the final output and is refined after each import round below.
  c2_cmd=("$PYTHON" "$BUILD_INDEX" "$c1_summaries" --project-kind "$PROJECT_KIND" --build-reason c1-raw -o "$index_file")
  echo "    C2 build index from raw summaries"
  printf 'C2: ' >>"$log_file"; printf '%q ' "${c2_cmd[@]}" >>"$log_file"; echo >>"$log_file"
  if ! run_and_capture "$c2_txt" "${c2_cmd[@]}"; then
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
  import_rounds_required=1
  if [[ -f "$index_for_next_round" ]] && ! project_index_has_importable_edges "$index_for_next_round"; then
    import_rounds_required=0
    echo "    import rounds skipped: index has no imported cross-TU summary edges"
    echo "import rounds skipped: local-only project index" >>"$log_file"

    # C6g5t: consume the C2 canonical local-only materialization once for
    # final reporting, but do not enter the recursive import/index rebuild loop.
    local_overlay_tag="local-index-overlay"
    local_overlay_text="$rounds_root/${local_overlay_tag}-text"
    local_overlay_summaries="$rounds_root/${local_overlay_tag}-summaries"
    local_overlay_reports="$rounds_root/${local_overlay_tag}-reports"
    local_overlay_paper_facts="$rounds_root/${local_overlay_tag}-paper-facts"
    local_overlay_runtime_metrics="$rounds_root/${local_overlay_tag}-runtime-metrics"
    local_overlay_summary_tmp_root="$rounds_root/${local_overlay_tag}-summary-tmp"
    local_overlay_report_tmp_root="$rounds_root/${local_overlay_tag}-report-tmp"
    mkdir -p "$local_overlay_text" "$local_overlay_summaries"
    if [[ "$SUMMARY_REPORTS" == "1" ]]; then
      mkdir -p "$local_overlay_reports" "$local_overlay_report_tmp_root"
    fi
    if [[ "$PAPER_FACTS" == "1" ]]; then
      mkdir -p "$local_overlay_paper_facts"
    fi
    if [[ "$RUNTIME_METRICS" == "1" ]]; then
      mkdir -p "$local_overlay_runtime_metrics"
    fi

    echo "    C2 local-only final-report overlay"
    for src in "${sources[@]}"; do
      stem="$(source_stem "$project" "$src")"
      out_txt="$local_overlay_text/${stem}.txt"
      src_summary_dir="$native_artifact_tmp_root/${local_overlay_tag}/${stem}/sidecars"
      rm -rf "$src_summary_dir"
      mkdir -p "$src_summary_dir"
      cmd=("$TOOL" --enable-summary-import --summary-index "$index_for_next_round" --summary-dir "$src_summary_dir")
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        src_report_dir="$native_artifact_tmp_root/${local_overlay_tag}/${stem}/reports"
        rm -rf "$src_report_dir"
        mkdir -p "$src_report_dir"
        cmd+=(--summary-report-dir "$src_report_dir")
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        cmd+=(--paper-facts-dir "$src_summary_dir")
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        cmd+=(--runtime-timing-dir "$src_summary_dir"
              --runtime-timing-project-id "$safe"
              --runtime-timing-stage "$local_overlay_tag")
      fi
      if [[ "$SUMMARY_IMPORT_DEBUG" == "1" ]]; then
        cmd+=(--summary-import-debug)
      fi
      if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
        cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-$local_overlay_tag")
      fi
      cmd+=("${CLANG_RESOURCE_DIR_TOOL_ARGS[@]}")
      cmd+=(-p "$project_dir" "$src")
      printf '%s: ' "$local_overlay_tag" >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
      if ! run_and_capture "$out_txt" "${cmd[@]}"; then
        echo "    $local_overlay_tag failed: $src"
        append_file_to_log "$log_file" "$local_overlay_tag failure output for ${src##*/}" "$out_txt"
        project_failed=1
      else
        if ! publish_source_summaries "$src_summary_dir" "$local_overlay_summaries" "$stem" "$log_file" "$local_overlay_tag ${src##*/}"; then
          project_failed=1
        fi
        if [[ "$SUMMARY_REPORTS" == "1" ]] && ! publish_source_reports "$src_report_dir" "$local_overlay_reports" "$stem" "$log_file" "$local_overlay_tag ${src##*/}"; then
          project_failed=1
        fi
        if [[ "$PAPER_FACTS" == "1" ]] && ! publish_source_paper_facts "$src_summary_dir" "$local_overlay_paper_facts" "$stem" "$log_file" "$local_overlay_tag ${src##*/}"; then
          project_failed=1
        fi
        if [[ "$RUNTIME_METRICS" == "1" ]] && ! publish_source_runtime_metrics "$src_summary_dir" "$local_overlay_runtime_metrics" "$stem" "$log_file" "$local_overlay_tag ${src##*/}"; then
          project_failed=1
        fi
      fi
    done
    latest_text="$local_overlay_text"
    latest_summaries="$local_overlay_summaries"
    latest_reports="$local_overlay_reports"
    latest_paper_facts="$local_overlay_paper_facts"
    latest_runtime_metrics="$local_overlay_runtime_metrics"
  fi
  if [[ -f "$index_for_next_round" && "$import_rounds_required" == "1" ]]; then
    for ((round = 1; round <= SUMMARY_IMPORT_ROUNDS; round++)); do
      round_tag="$(printf 'import-round-%02d' "$round")"
      round_text="$rounds_root/${round_tag}-text"
      round_summaries="$rounds_root/${round_tag}-summaries"
      round_reports="$rounds_root/${round_tag}-reports"
      round_paper_facts="$rounds_root/${round_tag}-paper-facts"
      round_runtime_metrics="$rounds_root/${round_tag}-runtime-metrics"
      round_report_tmp_root="$rounds_root/${round_tag}-report-tmp"
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
        mkdir -p "$round_reports" "$round_report_tmp_root"
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
        src_summary_dir="$native_artifact_tmp_root/${round_tag}/${stem}/sidecars"
        rm -rf "$src_summary_dir"
        mkdir -p "$src_summary_dir"
        cmd=("$TOOL" --enable-summary-import --summary-index "$round_index_input" --summary-dir "$src_summary_dir")
        if [[ "$SUMMARY_REPORTS" == "1" ]]; then
          src_report_dir="$native_artifact_tmp_root/${round_tag}/${stem}/reports"
          rm -rf "$src_report_dir"
          mkdir -p "$src_report_dir"
          cmd+=(--summary-report-dir "$src_report_dir")
        fi
        if [[ "$PAPER_FACTS" == "1" ]]; then
          cmd+=(--paper-facts-dir "$src_summary_dir")
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
        cmd+=("${CLANG_RESOURCE_DIR_TOOL_ARGS[@]}")
        cmd+=(-p "$project_dir" "$src")
        echo "      ${round_tag} ${src##*/}"
        printf '%s: ' "$round_tag" >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
        if ! run_and_capture "$out_txt" "${cmd[@]}"; then
          echo "    ${round_tag} failed: $src"
          append_file_to_log "$log_file" "${round_tag} failure output for ${src##*/}" "$out_txt"
          project_failed=1
        else
          if ! publish_source_summaries "$src_summary_dir" "$round_summaries" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
            echo "    ${round_tag} summary publish failed: $src"
            project_failed=1
          fi
          if [[ "$SUMMARY_REPORTS" == "1" ]] && ! publish_source_reports "$src_report_dir" "$round_reports" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
            echo "    ${round_tag} report publish failed: $src"
            project_failed=1
          fi
          if [[ "$PAPER_FACTS" == "1" ]] && ! publish_source_paper_facts "$src_summary_dir" "$round_paper_facts" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
            echo "    ${round_tag} paper-facts publish failed: $src"
            project_failed=1
          fi
          if [[ "$RUNTIME_METRICS" == "1" ]] && ! publish_source_runtime_metrics "$src_summary_dir" "$round_runtime_metrics" "$stem" "$log_file" "${round_tag} ${src##*/}"; then
            echo "    ${round_tag} runtime-timing publish failed: $src"
            project_failed=1
          fi
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
        rebuild_cmd=("$PYTHON" "$BUILD_INDEX" "$round_summaries" --project-kind "$PROJECT_KIND" --build-reason "${round_tag}-import-aware" --reuse-output-if-input-unchanged "$index_file" -o "$index_file")
        echo "    ${round_tag} rebuild index from import-aware summaries"
        printf 'rebuild-index-after-%s: ' "$round_tag" >>"$log_file"
        printf '%q ' "${rebuild_cmd[@]}" >>"$log_file"; echo >>"$log_file"
        if ! run_and_capture "$round_index_log" "${rebuild_cmd[@]}"; then
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
  elif [[ ! -f "$index_for_next_round" ]]; then
    echo "    import rounds skipped: missing index $index_for_next_round"
    project_failed=1
  fi

  # Publish latest/final import-aware outputs to the default project directory.
  # If SUMMARY_IMPORT_ROUNDS=0, this publishes C1 outputs instead.
  for src in "${sources[@]}"; do
    stem="$(source_stem "$project" "$src")"
    if [[ -f "$latest_text/${stem}.txt" ]]; then
      cp "$latest_text/${stem}.txt" "$final_text/${stem}.txt"
    fi
  done

  if compgen -G "$latest_summaries/*.cast-summary.json" >/dev/null; then
    cp "$latest_summaries"/*.cast-summary.json "$final_summaries"/
  fi
  if [[ "$SUMMARY_REPORTS" == "1" ]] && compgen -G "$latest_reports/*.cast-report.json" >/dev/null; then
    cp "$latest_reports"/*.cast-report.json "$final_reports"/
  fi
  if [[ "$PAPER_FACTS" == "1" ]] && compgen -G "$latest_paper_facts/*.cast-paper-facts.json" >/dev/null; then
    cp "$latest_paper_facts"/*.cast-paper-facts.json "$final_paper_facts"/
  fi
  if [[ "$RUNTIME_METRICS" == "1" ]] && compgen -G "$latest_runtime_metrics/*.cast-runtime.json" >/dev/null; then
    cp "$latest_runtime_metrics"/*.cast-runtime.json "$final_runtime_metrics"/
  fi
  if [[ "$RUNTIME_METRICS" == "1" ]] && compgen -G "$final_runtime_metrics/*.cast-runtime.json" >/dev/null; then
    runtime_cmd=("$PYTHON" "$RUNTIME_AGGREGATOR" "$final_runtime_metrics"
                 --per-tu-csv "$project_out/runtime-duration-by-tu.csv"
                 --per-project-csv "$project_out/runtime-duration-by-project.csv"
                 --latex-out "$project_out/runtime-duration.tex")
    if ! run_and_capture "$project_out/runtime-timing-aggregation.txt" "${runtime_cmd[@]}"; then
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

  echo "    completed import rounds: $completed_import_rounds"
  if [[ "$RUNTIME_METRICS" == "1" ]]; then
    echo "    runtime timings: $project_out/runtime-duration-by-project.csv"
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
    global_runtime_cmd=("$PYTHON" "$RUNTIME_AGGREGATOR" "${final_runtime_dirs[@]}"
                        --per-tu-csv "$OUTPUT_ROOT/runtime-duration-by-tu.csv"
                        --per-project-csv "$OUTPUT_ROOT/runtime-duration-by-project.csv"
                        --latex-out "$OUTPUT_ROOT/runtime-duration.tex")
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

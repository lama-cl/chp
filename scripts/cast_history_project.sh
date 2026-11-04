#!/usr/bin/env bash
# cast_history_project.sh - build the cast history of one or MANY C/C++ projects
# (in-tree or out-of-tree) into a chosen output directory.
#
# Clean replacement for run_all_multitu_projects-rp.sh when the benchmarks live
# outside this repository.  Per project it runs the same workflow:
#
#   C1  analyze every translation unit (TU)  -> per-TU cast-summary.json
#   C2  build project-summary-index.json     <- all C1 summaries
#   IMP N import-aware rounds: re-analyze each TU with --enable-summary-import,
#       rebuild the index from the new summaries (default N=1)
#   FIN publish final text + summaries + index
#
# Usage:
#   cast_history_project.sh [options] [INPUT...]
#
#   INPUT is a project directory, a directory that CONTAINS several projects,
#   or a compile_commands.json file.  Default: the current directory.
#
# Required inputs (everything else is auto-detected; see --help):
#   1. cast-history-tool binary   --tool, $TOOL, ./cast-history-tool,
#                                 ./build/ or <repo>/build/
#   2. compile_commands.json      per project: --compdb, INPUT/{,build/}compile_commands.json,
#                                 or --synthesize-compdb for self-contained sources
#   3. a clang whose builtin headers match the tool's LLVM (stddef.h, ...)
#                                 --resource-dir / --clang, or `clang` on PATH
#   4. python3 + scripts/build_summary_index.py (found next to this script)
#
# Exit status: 0 every project fully analyzed
#              1 finished, but some TU failed / used a stale fallback / a project was skipped
#              2 usage or environment problem (nothing was analyzed)

set -uo pipefail

if (( BASH_VERSINFO[0] < 4 )); then
  echo "error: bash >= 4 is required (found ${BASH_VERSION})." >&2
  echo "       on macOS: 'brew install bash' and run with /opt/homebrew/bin/bash." >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SCRIPT_NAME="$(basename "$0")"
SCRIPT_PID=$$     # the heartbeat helper exits when this process is gone
TOOL_PID=""       # analyzer currently running in the background
HB_PID=""         # its heartbeat helper
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"

# ---------------------------------------------------------------- logging ---
if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
  C_RED=$'\033[1;31m'; C_YEL=$'\033[33m'; C_GRN=$'\033[32m'; C_BLD=$'\033[1m'; C_OFF=$'\033[0m'
else
  C_RED=""; C_YEL=""; C_GRN=""; C_BLD=""; C_OFF=""
fi
RUN_LOG=""        # <output>/run.log   (whole run)
LOG_FILE=""       # <output>/<project>/commands.log (current project)
WARN_COUNT=0      # warnings in the current project (reset per project)
TOTAL_WARN=0

_log_file() {
  [[ -n "$RUN_LOG" ]] && printf '%s\n' "$*" >>"$RUN_LOG"
  [[ -n "$LOG_FILE" ]] && printf '%s\n' "$*" >>"$LOG_FILE"
  return 0
}
info()  { printf '%s\n' "$*"; _log_file "$*"; }
step()  { printf '%s==> %s%s\n' "$C_BLD" "$*" "$C_OFF"; _log_file "==> $*"; }
ok()    { printf '%s%s%s\n' "$C_GRN" "$*" "$C_OFF"; _log_file "$*"; }
warn()  { WARN_COUNT=$((WARN_COUNT + 1)); TOTAL_WARN=$((TOTAL_WARN + 1)); printf '%swarning:%s %s\n' "$C_YEL" "$C_OFF" "$*" >&2; _log_file "warning: $*"; }
loud()  { WARN_COUNT=$((WARN_COUNT + 1)); TOTAL_WARN=$((TOTAL_WARN + 1)); printf '%s!!! WARNING: %s%s\n' "$C_RED" "$*" "$C_OFF" >&2; _log_file "!!! WARNING: $*"; }
hint()  { printf '         %s\n' "$*" >&2; _log_file "         $*"; }
err()   { printf '%serror:%s %s\n' "$C_RED" "$C_OFF" "$*" >&2; _log_file "error: $*"; }
die()   { err "$1"; shift; local h; for h in "$@"; do hint "$h"; done; exit 2; }

# ------------------------------------------------------------------ usage ---
usage() {
  cat <<EOF
Usage: $SCRIPT_NAME [options] [INPUT...]

Build the cast history of the project(s) found in INPUT (default: current dir).
INPUT may be a project directory, a directory containing several projects
(each immediate subdirectory with a compile_commands.json is one project), or a
compile_commands.json file.

Output:
  -o, --output DIR        output root (default: ./cast-history-out); each project
                          is written to DIR/<project-name>/
      --force             allow DIR to be non-empty and not created by this script;
                          its CONTENTS ARE DELETED
Inputs:
      --compdb PATH       compile_commands.json (or its directory); single project only
      --synthesize-compdb for projects without a compile database, generate a simple
                          one from a recursive source scan (flags are guesses: see
                          --std / --cflags). Also makes a root directory with
                          source-only subdirectories count as several projects.
      --std STD           -std= for synthesized entries (default: c11 / c++17)
      --cflags "FLAGS"    extra flags for synthesized entries, e.g. "-Iinclude -DNDEBUG"
      --source-root DIR   base for output file names (default: the project directory,
                          or the common parent of its sources)
      --filter REGEX      keep only source paths matching this Python regex
      --exclude REGEX     drop source paths matching this Python regex
                          e.g. '/(test|tests|third_party|vendor|build)/'
      --extensions "..."  source suffixes (default: ".c .cc .cpp .cxx")
Tool / toolchain:
      --tool PATH         cast-history-tool binary (env: TOOL)
      --build-index PATH  build_summary_index.py (env: BUILD_INDEX)
      --clang CMD         clang used to find the builtin-header dir (env: CLANG_BIN)
      --resource-dir DIR  clang resource dir to inject (env: RESOURCE_DIR)
      --no-resource-dir   do not inject -resource-dir (only if your compdb has it)
      --python CMD        python interpreter (env: PYTHON, default python3)
Workflow:
      --rounds N          import/index refinement rounds (default 1; 0 = C1 only)
      --single-tu         analyze every TU independently: no index, no import rounds
                          (for corpora of unrelated single-file programs)
      --config-id ID      value for --summary-config-id
      --reports           also emit verbose *.cast-report.json sidecars (opt-in)
      --paper-facts       also emit compact *.cast-paper-facts.json sidecars (opt-in)
Metrics and sidecars (ON by default; each can be disabled):
      --no-runtime-metrics     skip *.cast-runtime.json timing sidecars and the CSV/LaTeX
                               timing tables (env: RUNTIME_METRICS=0)
      --no-prevalence-metrics  skip *.cast-prevalence.json pointer/cast prevalence
                               sidecars (env: PREVALENCE_METRICS=0)
      --no-partial-sidecars    do not keep sidecars + status manifests of TUs that fail
                               in C1 (env: PUBLISH_PARTIAL_C1_SIDECARS=0)
      --minimal                shorthand for all three --no-* options above
      --prevalence-return-types  also count pointer-typed function return definitions
      --normalize-paths        replace tool/project/output roots in the text reports with
                               <tool-root>, <project-root>, <output-root> (for diffing)
      --heartbeat SEC     print a "still running" line every SEC seconds for a slow
                          TU (default 30; 0 = off)
      --tu-timeout SEC    kill an analyzer that runs longer than SEC seconds and count the
                          TU as failed (default 0 = no limit; needs coreutils `timeout`)
      --stop-on-fail      stop the whole run at the first TU that fails
      --keep-intermediates keep per-phase work files under <project>/.work
  -h, --help              this help

Output layout:
  OUTPUT/run.log                         everything printed in this run
  OUTPUT/projects-summary.tsv            one row per project (status, counts)
  OUTPUT/<project>/<stem>.txt            final human-readable report per TU
  OUTPUT/<project>/summaries/<stem>.cast-summary.json   final machine-readable summaries
  OUTPUT/<project>/project-summary-index.json           index over the final summaries
  OUTPUT/<project>/commands.log          every command + failure output + warnings
  OUTPUT/<project>/failed-tus.txt        only if something failed
  OUTPUT/<project>/failed/<stem>.<phase>.txt   analyzer output of each failed TU
  OUTPUT/<project>/runtime-metrics/*.cast-runtime.json  per-TU timing sidecars (final pass)
  OUTPUT/<project>/runtime-duration-by-{tu,project}.csv, runtime-duration.tex
  OUTPUT/<project>/prevalence-metrics/*.cast-prevalence.json  source-level prevalence (C1)
  OUTPUT/<project>/partial-{summaries,reports,paper-facts,runtime-metrics,prevalence-metrics}/
                                         sidecars of TUs that failed in C1 (never indexed)
  OUTPUT/<project>/partial-status/<stem>.cast-analysis-status.json  one manifest per such TU
  OUTPUT/runtime-duration-*.csv/.tex     same tables aggregated over all projects
  <stem> is the source path relative to the source root with '/' replaced by '_'.
  A TU that fails in C1 is left out of the index and not retried.  A TU that
  fails in an import round falls back to its previous-phase summary/report
  (the report gets a STALE banner) and is flagged loudly.

Examples:
  # one project with build/compile_commands.json
  $SCRIPT_NAME -o /data/out /data/benchmarks/lua-5.4.1

  # a directory holding many projects (each with a compile_commands.json)
  $SCRIPT_NAME -o /data/out /data/benchmarks

  # source-only benchmark corpus, no build metadata, independent programs
  $SCRIPT_NAME --synthesize-compdb --single-tu -o /data/out /data/bench-genc2rust
EOF
}

# ------------------------------------------------------------- arguments ---
INPUTS=()
OUTPUT_DIR="./cast-history-out"
FORCE=0
COMPDB_ARG=""
SYNTH=0
SYNTH_STD=""
SYNTH_CFLAGS=""
SOURCE_ROOT=""
SRC_FILTER=""
SRC_EXCLUDE=""
SRC_EXT=".c .cc .cpp .cxx"
TOOL="${TOOL:-}"
BUILD_INDEX="${BUILD_INDEX:-}"
CLANG_BIN="${CLANG_BIN:-}"
RESOURCE_DIR="${RESOURCE_DIR:-}"
NO_RESOURCE_DIR=0
PYTHON="${PYTHON:-python3}"
ROUNDS=1
SINGLE_TU=0
CONFIG_ID=""
REPORTS=0
PAPER_FACTS=0
HEARTBEAT=30
TU_TIMEOUT=0
STOP_ON_FAIL=0
KEEP=0
RUNTIME_METRICS="${RUNTIME_METRICS:-1}"
PREVALENCE_METRICS="${PREVALENCE_METRICS:-1}"
PUBLISH_PARTIAL_C1_SIDECARS="${PUBLISH_PARTIAL_C1_SIDECARS:-1}"
PREVALENCE_RETURN_TYPES="${PREVALENCE_INCLUDE_RETURN_TYPES:-0}"
NORMALIZE_PATHS="${NORMALIZE_PATHS:-0}"
RUNTIME_AGGREGATOR="${RUNTIME_AGGREGATOR:-}"

ORIG_ARGS=("$@")
# Allow --opt=value as well as --opt value.
ARGS=()
for a in "$@"; do
  if [[ "$a" == --*=* ]]; then ARGS+=("${a%%=*}" "${a#*=}"); else ARGS+=("$a"); fi
done
set -- ${ARGS[@]+"${ARGS[@]}"}

need_value() { [[ $# -ge 2 && -n "$2" ]] || die "option $1 needs a value" "run '$SCRIPT_NAME --help' for usage"; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)            usage; exit 0 ;;
    -o|--output)          need_value "$@"; OUTPUT_DIR="$2"; shift 2 ;;
    --force)              FORCE=1; shift ;;
    --compdb)             need_value "$@"; COMPDB_ARG="$2"; shift 2 ;;
    --synthesize-compdb)  SYNTH=1; shift ;;
    --std)                need_value "$@"; SYNTH_STD="$2"; shift 2 ;;
    --cflags)             need_value "$@"; SYNTH_CFLAGS="$2"; shift 2 ;;
    --source-root)        need_value "$@"; SOURCE_ROOT="$2"; shift 2 ;;
    --filter)             need_value "$@"; SRC_FILTER="$2"; shift 2 ;;
    --exclude)            need_value "$@"; SRC_EXCLUDE="$2"; shift 2 ;;
    --extensions)         need_value "$@"; SRC_EXT="$2"; shift 2 ;;
    --tool)               need_value "$@"; TOOL="$2"; shift 2 ;;
    --build-index)        need_value "$@"; BUILD_INDEX="$2"; shift 2 ;;
    --clang)              need_value "$@"; CLANG_BIN="$2"; shift 2 ;;
    --resource-dir)       need_value "$@"; RESOURCE_DIR="$2"; shift 2 ;;
    --no-resource-dir)    NO_RESOURCE_DIR=1; shift ;;
    --python)             need_value "$@"; PYTHON="$2"; shift 2 ;;
    --rounds)             need_value "$@"; ROUNDS="$2"; shift 2 ;;
    --single-tu)          SINGLE_TU=1; shift ;;
    --config-id)          need_value "$@"; CONFIG_ID="$2"; shift 2 ;;
    --reports)            REPORTS=1; shift ;;
    --paper-facts)        PAPER_FACTS=1; shift ;;
    --heartbeat)          need_value "$@"; HEARTBEAT="$2"; shift 2 ;;
    --tu-timeout)         need_value "$@"; TU_TIMEOUT="$2"; shift 2 ;;
    --no-runtime-metrics)    RUNTIME_METRICS=0; shift ;;
    --no-prevalence-metrics) PREVALENCE_METRICS=0; shift ;;
    --no-partial-sidecars)   PUBLISH_PARTIAL_C1_SIDECARS=0; shift ;;
    --minimal)               RUNTIME_METRICS=0; PREVALENCE_METRICS=0; PUBLISH_PARTIAL_C1_SIDECARS=0; shift ;;
    --prevalence-return-types) PREVALENCE_RETURN_TYPES=1; shift ;;
    --normalize-paths)       NORMALIZE_PATHS=1; shift ;;
    --stop-on-fail)       STOP_ON_FAIL=1; shift ;;
    --keep-intermediates) KEEP=1; shift ;;
    -*)  die "unknown option: $1" "run '$SCRIPT_NAME --help' for the list of options" ;;
    *)   INPUTS+=("$1"); shift ;;
  esac
done
[[ ${#INPUTS[@]} -gt 0 ]] || INPUTS=(".")

[[ "$ROUNDS" =~ ^[0-9]+$ ]] || die "--rounds must be a non-negative integer, got '$ROUNDS'"
for v in RUNTIME_METRICS PREVALENCE_METRICS PUBLISH_PARTIAL_C1_SIDECARS PREVALENCE_RETURN_TYPES NORMALIZE_PATHS; do
  [[ "${!v}" == 0 || "${!v}" == 1 ]] || die "$v must be 0 or 1, got '${!v}'"
done
[[ "$HEARTBEAT" =~ ^[0-9]+$ ]] || die "--heartbeat must be a non-negative integer (seconds), got '$HEARTBEAT'"
[[ "$TU_TIMEOUT" =~ ^[0-9]+$ ]] || die "--tu-timeout must be a non-negative integer (seconds), got '$TU_TIMEOUT'"
TIMEOUT_CMD=""
if (( TU_TIMEOUT > 0 )); then
  for c in timeout gtimeout; do command -v "$c" >/dev/null 2>&1 && { TIMEOUT_CMD="$c"; break; }; done
  [[ -n "$TIMEOUT_CMD" ]] || die "--tu-timeout needs the coreutils 'timeout' command, which was not found" \
    "install coreutils (macOS: brew install coreutils, which provides gtimeout) or drop --tu-timeout"
fi
if (( SINGLE_TU && ROUNDS != 1 )); then
  info "note: --single-tu ignores --rounds"
fi

# ------------------------------------------------------------- pre-flight ---
step "Pre-flight checks"

command -v "$PYTHON" >/dev/null 2>&1 || die "python interpreter not found: $PYTHON" \
  "install python3 or pass --python /path/to/python3"

# Tool binary.
if [[ -z "$TOOL" ]]; then
  for cand in ./cast-history-tool ./build/cast-history-tool "$REPO_ROOT/build/cast-history-tool"; do
    if [[ -x "$cand" && -f "$cand" ]]; then TOOL="$cand"; break; fi
  done
fi
[[ -n "$TOOL" ]] || die "cast-history-tool binary not found" \
  "build it:  cmake -S $REPO_ROOT -B $REPO_ROOT/build -DLLVM_DIR=<llvm>/lib/cmake/llvm -DClang_DIR=<llvm>/lib/cmake/clang && cmake --build $REPO_ROOT/build -j" \
  "then pass --tool $REPO_ROOT/build/cast-history-tool (or export TOOL=...)"
[[ -f "$TOOL" && -x "$TOOL" ]] || die "tool is missing or not executable: $TOOL" \
  "check the path or run: chmod +x $TOOL"
TOOL="$(cd "$(dirname "$TOOL")" && pwd -P)/$(basename "$TOOL")"
if ! TOOL_HELP="$("$TOOL" --help 2>&1)"; then
  err "tool does not start: $TOOL"
  printf '%s\n' "$TOOL_HELP" | head -5 | sed 's/^/         /' >&2
  hint "typical cause: LLVM shared libraries not on the loader path (set LD_LIBRARY_PATH)"
  hint "or the binary was built for another platform; rebuild it here."
  exit 2
fi
for opt in --summary-dir --enable-summary-import --summary-index; do
  grep -q -- "$opt" <<<"$TOOL_HELP" || die "tool does not support $opt: $TOOL" \
    "it is older than this script; rebuild from the current source tree"
done
TOOL_LLVM_MAJOR="$("$TOOL" --version 2>/dev/null | sed -n 's/.*LLVM version \([0-9][0-9]*\)\..*/\1/p' | head -1)"
info "tool:           $TOOL (LLVM ${TOOL_LLVM_MAJOR:-version unknown})"

# Index builder.
BUILD_INDEX="${BUILD_INDEX:-$SCRIPT_DIR/build_summary_index.py}"
if (( ! SINGLE_TU )); then
  [[ -f "$BUILD_INDEX" ]] || die "index builder not found: $BUILD_INDEX" \
    "it normally sits next to this script; otherwise pass --build-index /path/to/build_summary_index.py"
  info "index builder:  $BUILD_INDEX"
fi

# Runtime-timing aggregator (only needed when runtime metrics are on).
if (( RUNTIME_METRICS )); then
  RUNTIME_AGGREGATOR="${RUNTIME_AGGREGATOR:-$SCRIPT_DIR/aggregate_runtime_metrics.py}"
  [[ -f "$RUNTIME_AGGREGATOR" ]] || die "runtime timing aggregator not found: $RUNTIME_AGGREGATOR" \
    "it normally sits next to this script; set RUNTIME_AGGREGATOR=/path/to/aggregate_runtime_metrics.py" \
    "or run with --no-runtime-metrics"
fi
info "metrics:        runtime=$RUNTIME_METRICS prevalence=$PREVALENCE_METRICS partial-sidecars=$PUBLISH_PARTIAL_C1_SIDECARS"

# Clang resource directory (builtin headers: stddef.h, stdarg.h, ...).
# Compile databases normally omit -resource-dir because the clang driver adds it
# implicitly; LibTooling run with -p does not, so every TU would fail with
# "'stddef.h' file not found" unless it is injected.
RESOURCE_ARGS=()
if (( NO_RESOURCE_DIR )); then
  warn "--no-resource-dir: builtin headers must come from the compile database flags"
else
  if [[ -z "$RESOURCE_DIR" ]]; then
    if [[ -z "$CLANG_BIN" ]]; then
      # Prefer the clang that matches the LLVM the tool was built with.
      for c in ${TOOL_LLVM_MAJOR:+clang-$TOOL_LLVM_MAJOR} clang clang-22 clang-21 clang-20 clang-19 clang-18 clang-17 clang-16 clang-15 clang-14; do
        if command -v "$c" >/dev/null 2>&1; then CLANG_BIN="$c"; break; fi
      done
    fi
    [[ -n "$CLANG_BIN" ]] && command -v "$CLANG_BIN" >/dev/null 2>&1 || die \
      "no clang found to locate the builtin headers (${CLANG_BIN:-searched clang, clang-14..22})" \
      "install clang, or pass --clang /path/to/clang (same LLVM version as the tool)" \
      "or pass --resource-dir <dir containing include/stddef.h>"
    RESOURCE_DIR="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)" || die \
      "'$CLANG_BIN -print-resource-dir' failed" "pass --resource-dir explicitly"
    info "clang:          $CLANG_BIN ($("$CLANG_BIN" --version 2>/dev/null | head -1))"
  fi
  # Builtin headers from another LLVM major version can fail to parse or change semantics.
  rd_major="$(basename "$RESOURCE_DIR" | sed -n 's/^\([0-9][0-9]*\).*/\1/p')"
  if [[ -n "$TOOL_LLVM_MAJOR" && -n "$rd_major" && "$rd_major" != "$TOOL_LLVM_MAJOR" ]]; then
    warn "resource dir is for LLVM $rd_major but the tool was built with LLVM $TOOL_LLVM_MAJOR: $RESOURCE_DIR"
    hint "pass --clang clang-$TOOL_LLVM_MAJOR or --resource-dir <LLVM $TOOL_LLVM_MAJOR resource dir>"
  fi
  [[ -f "$RESOURCE_DIR/include/stddef.h" ]] || die \
    "invalid clang resource directory: $RESOURCE_DIR" \
    "expected builtin header $RESOURCE_DIR/include/stddef.h" \
    "use the resource dir of the clang matching the LLVM the tool was built with (--resource-dir / --clang)"
  RESOURCE_ARGS=("--extra-arg-before=-resource-dir=$RESOURCE_DIR")
  info "resource dir:   $RESOURCE_DIR"
fi

# Output root.  We only ever delete a directory this script created (marker
# file) or one the user explicitly allowed with --force.
mkdir -p "$OUTPUT_DIR" 2>/dev/null || die "cannot create output directory: $OUTPUT_DIR" \
  "check permissions and that the parent exists"
[[ -w "$OUTPUT_DIR" ]] || die "output directory is not writable: $OUTPUT_DIR" "check permissions"
ROOT_OUT="$(cd "$OUTPUT_DIR" && pwd -P)"
MARKER="$ROOT_OUT/.cast-history-output"
if [[ "$ROOT_OUT" == "/" || "$ROOT_OUT" == "${HOME:-/nonexistent}" ]]; then
  die "refusing to use $ROOT_OUT as output directory" "choose a dedicated directory, e.g. -o $ROOT_OUT/cast-history-out"
fi
if [[ -n "$(ls -A "$ROOT_OUT")" && ! -f "$MARKER" ]]; then
  (( FORCE )) || die "output directory is not empty and was not created by this script: $ROOT_OUT" \
    "choose a new/empty directory, or pass --force to overwrite its contents"
fi
# Never wipe a directory that holds (or is) one of the inputs.
for in in "${INPUTS[@]}"; do
  if [[ -e "$in" ]]; then
    in_abs="$(cd "$(dirname "$in")" && pwd -P)/$(basename "$in")"; [[ -d "$in" ]] && in_abs="$(cd "$in" && pwd -P)"
    if [[ "$in_abs" == "$ROOT_OUT" || "$in_abs/" == "$ROOT_OUT/"* ]]; then
      die "input $in is, or lies inside, the output directory $ROOT_OUT" \
        "its contents would be deleted; choose an output directory outside the inputs (e.g. -o $ROOT_OUT/../cast-history-out)"
    fi
  fi
done
find "$ROOT_OUT" -mindepth 1 -delete 2>/dev/null
echo "created by $SCRIPT_NAME on $(date -u +%FT%TZ)" >"$MARKER"
RUN_LOG="$ROOT_OUT/run.log"
{
  echo "# $SCRIPT_NAME  $(date -u +%FT%TZ)"
  printf '# argv:'; printf ' %q' ${ORIG_ARGS[@]+"${ORIG_ARGS[@]}"}; echo
  echo "# cwd: $(pwd -P)"
  echo "# tool: $TOOL"
  echo "# resource dir: ${RESOURCE_DIR:-none}"
  if command -v git >/dev/null 2>&1; then
    echo "# tool-repo git commit: $(git -C "$REPO_ROOT" rev-parse HEAD 2>/dev/null || echo n/a)"
  fi
} >"$RUN_LOG"
info "output:         $ROOT_OUT"

esc_sed() { printf '%s' "$1" | sed -e 's/[][\/.*^$&]/\\&/g'; }
if (( NORMALIZE_PATHS )); then
  NORM_OUT_ESC="$(esc_sed "$ROOT_OUT")"; NORM_TOOL_ESC="$(esc_sed "$REPO_ROOT")"
fi

# ----------------------------------------------------- project discovery ---
export CH_WARN="${C_YEL}warning:${C_OFF}" CH_NOTE="note:"
export CH_EXT="$SRC_EXT" CH_FILTER="$SRC_FILTER" CH_EXCLUDE="$SRC_EXCLUDE"

P_DIR=(); P_COMPDB=(); P_NAME=()

# Echo the compile database that belongs to directory $1 (if any).
dir_compdb() {
  local d="$1"
  if   [[ -f "$d/compile_commands.json" ]];       then echo "$d/compile_commands.json"
  elif [[ -f "$d/build/compile_commands.json" ]]; then echo "$d/build/compile_commands.json"
  fi
}

# True if directory $1 contains source files (optionally only down to depth $2).
dir_has_sources() {
  local d="$1" depth="${2:-}" names=() e md=()
  for e in $SRC_EXT; do names+=(-o -iname "*$e"); done
  [[ -n "$depth" ]] && md=(-maxdepth "$depth")
  [[ -n "$(find "$d" -mindepth 1 ${md[@]+"${md[@]}"} \( -name '.*' -prune -o -type f \( "${names[@]:1}" \) -print -quit \) 2>/dev/null)" ]]
}

add_project() {  # add_project DIR COMPDB(or empty)
  local d="$1" cdb="$2" name base n=2
  name="$(basename "$d")"
  base="$name"
  while printf '%s\n' ${P_NAME[@]+"${P_NAME[@]}"} | grep -qx -- "$name"; do
    name="${base}_$n"; n=$((n + 1))
  done
  [[ "$name" == "$base" ]] || warn "two projects are both named '$base'; the second is written to '$name'"
  P_DIR+=("$d"); P_COMPDB+=("$cdb"); P_NAME+=("$name")
}

discover_projects() {
  local in dir cdb sub subs n_before dname
  for in in "${INPUTS[@]}"; do
    n_before=${#P_DIR[@]}
    if [[ -f "$in" ]]; then
      [[ "$(basename "$in")" == "compile_commands.json" ]] || die \
        "input is a file but not compile_commands.json: $in" \
        "pass a project directory, a directory of projects, or a file named exactly compile_commands.json"
      cdb="$(cd "$(dirname "$in")" && pwd -P)/compile_commands.json"
      dir="$(dirname "$cdb")"
      dname="$(basename "$dir")"
      case "$dname" in build|_build|out|cmake-build-*) dir="$(dirname "$dir")";; esac
      add_project "$dir" "$cdb"
      continue
    fi
    [[ -d "$in" ]] || die "input not found: $in" "check the path; relative paths are resolved from $(pwd -P)"
    dir="$(cd "$in" && pwd -P)"

    if [[ -n "$COMPDB_ARG" ]]; then
      if [[ -d "$COMPDB_ARG" ]]; then cdb="$COMPDB_ARG/compile_commands.json"; else cdb="$COMPDB_ARG"; fi
      [[ -f "$cdb" ]] || die "compile_commands.json not found at: $COMPDB_ARG" "pass the file or the directory that contains it"
      [[ "$(basename "$cdb")" == "compile_commands.json" ]] || die \
        "the tool only reads files named compile_commands.json, got: $(basename "$cdb")" \
        "copy or symlink it:  ln -s $(basename "$cdb") $(dirname "$cdb")/compile_commands.json"
      add_project "$dir" "$(cd "$(dirname "$cdb")" && pwd -P)/compile_commands.json"
      continue
    fi

    cdb="$(dir_compdb "$dir")"
    if [[ -n "$cdb" ]]; then add_project "$dir" "$cdb"; continue; fi

    # Not a project itself: is it a root holding several projects?
    mapfile -t subs < <(find "$dir" -mindepth 1 -maxdepth 1 -type d -not -name '.*' 2>/dev/null | sort)
    for sub in ${subs[@]+"${subs[@]}"}; do
      [[ "$sub" == "$ROOT_OUT" ]] && continue
      cdb="$(dir_compdb "$sub")"
      if [[ -n "$cdb" ]]; then add_project "$sub" "$cdb"
      elif (( SYNTH )) && dir_has_sources "$sub"; then add_project "$sub" ""
      elif dir_has_sources "$sub"; then
        warn "skipping $sub: it has sources but no compile_commands.json"
        hint "generate one (cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON / bear -- make) or use --synthesize-compdb"
      fi
    done
    (( ${#P_DIR[@]} > n_before )) && continue

    # A directory with sources of its own, and --synthesize-compdb: one project.
    if (( SYNTH )) && dir_has_sources "$dir"; then add_project "$dir" ""; continue; fi

    err "no project found in: $dir"
    hint "looked for compile_commands.json in $dir/, $dir/build/ and in each immediate subdirectory"
    hint "create one, then re-run (or pass --compdb for a single project):"
    hint "  CMake:      cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    hint "  make/other: bear -- make        (or: compiledb -n make)"
    hint "  self-contained sources only: re-run with --synthesize-compdb"
    exit 2
  done
}

echo
step "Discovering projects"
discover_projects
NPROJ=${#P_DIR[@]}
(( NPROJ > 0 )) || die "no projects to analyze" "see the messages above"
for ((i = 0; i < NPROJ; i++)); do
  info "  $(printf '%-24s' "${P_NAME[i]}") ${P_DIR[i]}  [${P_COMPDB[i]:-compile db will be synthesized}]"
done
if [[ -n "$COMPDB_ARG" && $NPROJ -gt 1 ]]; then
  die "--compdb applies to a single project but $NPROJ projects were found" "pass one project directory with --compdb"
fi

# ------------------------------------------------- compile_commands.json ---
synthesize_compdb() {  # synthesize_compdb PROJECT_DIR OUT_DIR  -> prints number of entries
  local project="$1" dir="$2"; mkdir -p "$dir"
  "$PYTHON" - "$dir/compile_commands.json" "$project" "$SYNTH_STD" "$SYNTH_CFLAGS" <<'PY'
import json, os, re, shlex, sys
out_file, project, std, cflags = sys.argv[1:5]
exts = tuple(e.lower() for e in os.environ["CH_EXT"].split())
flt = re.compile(os.environ["CH_FILTER"]) if os.environ["CH_FILTER"] else None
exc = re.compile(os.environ["CH_EXCLUDE"]) if os.environ["CH_EXCLUDE"] else None
extra = shlex.split(cflags)
entries, dirs = [], set()
for root, dnames, fnames in os.walk(project):
    dnames[:] = sorted(d for d in dnames if not d.startswith("."))
    for f in sorted(fnames):
        p = os.path.join(root, f)
        if not f.lower().endswith(exts): continue
        if flt and not flt.search(p): continue
        if exc and exc.search(p): continue
        entries.append(p); dirs.add(root)
if not entries:
    print(f"error: no source files with suffix {' '.join(exts)} found under {project}", file=sys.stderr)
    sys.exit(3)
incs = [f"-I{project}"] + [f"-I{d}" for d in sorted(dirs) if d != project]
out = []
for p in entries:
    cpp = not p.lower().endswith(".c")
    s = std or ("c++17" if cpp else "c11")
    out.append({"directory": project, "file": p,
                "arguments": ["clang++" if cpp else "clang", f"-std={s}", *incs, *extra, "-c", p, "-o", os.devnull]})
with open(out_file, "w", encoding="utf-8") as fh:
    json.dump(out, fh, indent=2); fh.write("\n")
print(f"{len(out)}")
PY
}

# ----------------------------------------------------------- run helpers ---
FAILED=()      # "phase<TAB>exit<TAB>source<TAB>reason<TAB>fallback"
SKIP=()        # SKIP[i]=1: TU failed in C1, so later passes do not retry it
STALE=()       # "tag<TAB>relpath": TUs whose final result is a previous-phase fallback
PASS_OK=0; PASS_FAIL=0; TU_OK=1
STOP_ALL=0

# Suggest a fix for the first clang error we saw.
hint_for_error() {
  local msg="$1"
  case "$msg" in
    *"'stddef.h' file not found"*|*"'stdarg.h' file not found"*|*"'stdbool.h' file not found"*|*"'stdint.h' file not found"*|*"'limits.h' file not found"*|*"'float.h' file not found"*)
      hint "builtin clang header missing: --resource-dir (${RESOURCE_DIR:-not set}) does not match the clang the tool was built with"
      hint "pass --resource-dir \$(clang-NN -print-resource-dir) for the right LLVM version" ;;
    *"file not found"*)
      hint "missing include: check the -I flags in compile_commands.json for this TU;"
      hint "generated headers (config.h, ...) exist only after the project's configure/build step has run." ;;
    *"unknown type name"*|*"undeclared identifier"*)
      hint "likely a missing -D macro or include path in compile_commands.json for this TU." ;;
  esac
}

# Prints a progress line every $HEARTBEAT seconds while analyzer $1 runs.  It ends on its
# own when the analyzer or this script disappears, so it can never be left behind.
heartbeat_loop() {  # heartbeat_loop TOOL_PID TAG REL T0
  local pid="$1" tag="$2" rel="$3" t0="$4" k
  while :; do
    for ((k = 0; k < HEARTBEAT; k++)); do
      sleep 1
      kill -0 "$pid" 2>/dev/null && kill -0 "$SCRIPT_PID" 2>/dev/null || return 0
    done
    printf '           ... %s %s still running (%ds)\n' "$tag" "$rel" $((SECONDS - t0))
  done
}

# Stop whatever we started in the background.  Runs on every exit path (EXIT trap).
cleanup_children() {
  local pid i
  for pid in "$HB_PID" "$TOOL_PID"; do
    [[ -n "$pid" ]] || continue
    kill "$pid" 2>/dev/null || continue
    for i in 1 2 3 4 5 6 7 8 9 10; do kill -0 "$pid" 2>/dev/null || break; sleep 0.2; done
    kill -KILL "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
  done
  HB_PID=""; TOOL_PID=""
  return 0
}

on_signal() {  # on_signal SIGNAL-NAME
  trap - INT TERM HUP
  echo; err "interrupted ($1): stopping the analyzer"
  [[ -n "${ROOT_OUT:-}" ]] && hint "partial results are in $ROOT_OUT"
  cleanup_children
  exit 130
}
trap 'on_signal INT' INT
trap 'on_signal TERM' TERM
trap 'on_signal HUP' HUP
trap cleanup_children EXIT

# Stage label stored in runtime sidecars (same names as the older runners).
stage_name() {
  case "$1" in
    c1) echo c1 ;;
    imp*) echo "import-round-${1#imp}" ;;
    local-overlay) echo local-index-overlay ;;
    *) echo "$1" ;;
  esac
}

normalize_file() {  # replace machine-specific roots in a text report
  local f="$1" proj_esc
  proj_esc="$(esc_sed "$PROJECT_DIR")"
  sed -e "s/$NORM_OUT_ESC/<output-root>/g" -e "s/$proj_esc/<project-root>/g" -e "s/$NORM_TOOL_ESC/<tool-root>/g" \
    "$f" >"$f.norm" && mv "$f.norm" "$f"
}

PARTIAL_ARTIFACTS=()
# publish_partial SRC_DIR DEST_DIR STEM SUFFIX KIND : keep a sidecar of a failed C1 TU
publish_partial() {
  local sdir="$1" ddir="$2" stem="$3" suffix="$4" kind="$5" files=() f i=0
  [[ -d "$sdir" ]] || return 0
  mapfile -t files < <(find "$sdir" -type f -name "*$suffix" 2>/dev/null | sort)
  for f in ${files[@]+"${files[@]}"}; do
    mkdir -p "$ddir"
    local dest="$ddir/${stem}${suffix}"; (( i > 0 )) && dest="$ddir/${stem}__extra_${i}${suffix}"
    cp "$f" "$dest" && PARTIAL_ARTIFACTS+=("$kind|$(basename "$ddir")/$(basename "$dest")")
    i=$((i + 1))
  done
  return 0
}

write_partial_status() {  # write_partial_status STEM SRC EXIT TEXT_NAME REASON
  local stem="$1" src="$2" code="$3" tname="$4" reason="$5"
  mkdir -p "$OUT/partial-status"
  "$PYTHON" - "$OUT/partial-status/${stem}.cast-analysis-status.json" "$src" "$stem" "$code" "$tname" "$SCRIPT_NAME" "$reason" ${PARTIAL_ARTIFACTS[@]+"${PARTIAL_ARTIFACTS[@]}"} <<'PY'
import json, pathlib, sys
status_path, source_file, stem, code, tname, runner, reason = sys.argv[1:8]
arts = []
for item in sys.argv[8:]:
    kind, sep, path = item.partition("|")
    if sep: arts.append({"kind": kind, "path": path})
payload = {"schemaVersion": 1, "sourceFile": source_file, "sourceStem": stem, "stage": "c1",
           "complete": False, "c2Eligible": False, "toolExitCode": int(code), "runner": runner,
           "capturedTextFile": tname, "reason": reason, "publishedArtifacts": arts}
pathlib.Path(status_path).write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
}

# publish_sidecar SRC_DIR DEST_DIR STEM SUFFIX LABEL  -> copies the emitted file(s) with a collision-safe name
publish_sidecar() {
  local sdir="$1" ddir="$2" stem="$3" suffix="$4" label="$5" files=() i=1 f
  mapfile -t files < <(find "$sdir" -type f -name "*$suffix" 2>/dev/null | sort)
  if [[ ${#files[@]} -eq 0 ]]; then
    warn "$label: tool exited 0 but wrote no *$suffix under $sdir"
    return 1
  fi
  mkdir -p "$ddir"
  cp "${files[0]}" "$ddir/${stem}${suffix}" || { err "$label: cannot copy ${files[0]} to $ddir"; return 1; }
  for f in "${files[@]:1}"; do
    cp "$f" "$ddir/${stem}__extra_${i}${suffix}"; i=$((i + 1))
  done
  (( ${#files[@]} > 1 )) && warn "$label: ${#files[@]} $suffix files emitted for one source; extras saved as ${stem}__extra_N${suffix}"
  return 0
}

# run_tu TAG IDX STEM SRC REL TEXT_DIR SUM_DIR REP_DIR PF_DIR RT_DIR [INDEX_FILE PREV_TAG PREV_SUM_DIR PREV_TEXT_DIR]
# Analyze one TU, publish its outputs, print one progress line.
run_tu() {
  local tag="$1" idx="$2" stem="$3" src="$4" rel="$5" tdir="$6" sdir_pub="$7" rdir_pub="$8" pdir_pub="$9"
  local rtdir_pub="${10:-}" index="${11:-}" prev_tag="${12:-}" prev_sum="${13:-}" prev_text="${14:-}" prev_json=""
  local tmp="$WORK/$tag-tmp/$stem" sdir rdir out_txt rc t0 nerr first status="ok"
  sdir="$tmp/s"; rdir="$tmp/r"; out_txt="$tdir/${stem}.txt"
  rm -rf "$tmp"; mkdir -p "$sdir" "$tdir"
  local cmd=("$TOOL")
  [[ -n "$index" ]] && cmd+=(--enable-summary-import --summary-index "$index")
  cmd+=(--summary-dir "$sdir")
  if (( REPORTS )); then mkdir -p "$rdir"; cmd+=(--summary-report-dir "$rdir"); fi
  (( PAPER_FACTS )) && cmd+=(--paper-facts-dir "$sdir" --paper-facts-project-root "$PROJECT_DIR")
  (( RUNTIME_METRICS )) && cmd+=(--runtime-timing-dir "$sdir" --runtime-timing-project-id "$P_NAME_CUR"
                                 --runtime-timing-stage "$(stage_name "$tag")")
  if (( PREVALENCE_METRICS )) && [[ "$tag" == c1 ]]; then   # source-level facts: C1 only
    prev_json="$OUT/prevalence-metrics/${stem}.cast-prevalence.json"; mkdir -p "$OUT/prevalence-metrics"
    cmd+=(--prevalence-metrics-json "$prev_json" --prevalence-metrics-project-root "$PROJECT_DIR"
          --prevalence-metrics-project-id "$P_NAME_CUR")
    (( PREVALENCE_RETURN_TYPES )) && cmd+=(--prevalence-include-return-types)
  fi
  [[ -n "$CONFIG_ID" ]] && cmd+=(--summary-config-id "$CONFIG_ID-$tag")
  cmd+=(${RESOURCE_ARGS[@]+"${RESOURCE_ARGS[@]}"} -p "$COMPDB_DIR" "$src")

  { printf '%s: ' "$tag"; printf '%q ' "${cmd[@]}"; echo; } >>"$LOG_FILE"
  t0=$SECONDS
  { echo "Command:"; printf '%q ' "${cmd[@]}"; echo; echo; echo "Output:"; } >"$out_txt"
  local run=("${cmd[@]}")
  (( TU_TIMEOUT > 0 )) && run=("$TIMEOUT_CMD" --kill-after=10 "$TU_TIMEOUT" "${cmd[@]}")
  # Run in the background and wait: bash then handles Ctrl-C/TERM immediately instead of
  # after the analyzer finishes, and cleanup_children can stop it.
  "${run[@]}" >>"$out_txt" 2>&1 &
  TOOL_PID=$!
  if (( HEARTBEAT > 0 )); then heartbeat_loop "$TOOL_PID" "$tag" "$rel" "$t0" & HB_PID=$!; fi
  wait "$TOOL_PID"; rc=$?
  TOOL_PID=""
  if [[ -n "$HB_PID" ]]; then kill "$HB_PID" 2>/dev/null; wait "$HB_PID" 2>/dev/null; HB_PID=""; fi
  nerr="$(grep -E -c ':[0-9]+:[0-9]+: (fatal )?error:' "$out_txt")"
  first="$(grep -E -m1 ':[0-9]+:[0-9]+: (fatal )?error:' "$out_txt" | sed -e "s#$PROJECT_DIR/##")"

  (( NORMALIZE_PATHS )) && normalize_file "$out_txt"

  TU_OK=0
  local reason=""
  if (( rc != 0 )); then
    status="FAILED (exit $rc)"
    reason="${first:-analyzer exited nonzero}"
    if (( TU_TIMEOUT > 0 && (rc == 124 || rc == 137) )); then
      status="FAILED (timeout)"; reason="analyzer killed after the ${TU_TIMEOUT}s limit (--tu-timeout)"
    fi
    { echo; echo "===== $tag failure output for $rel ====="; tail -n 40 "$out_txt"; } >>"$LOG_FILE"
  elif publish_sidecar "$sdir" "$sdir_pub" "$stem" ".cast-summary.json" "$tag $rel" \
       && { (( ! REPORTS )) || publish_sidecar "$rdir" "$rdir_pub" "$stem" ".cast-report.json" "$tag $rel"; } \
       && { (( ! PAPER_FACTS )) || publish_sidecar "$sdir" "$pdir_pub" "$stem" ".cast-paper-facts.json" "$tag $rel"; }; then
    PASS_OK=$((PASS_OK + 1)); TU_OK=1
    # Metric sidecars are best-effort: a missing one warns but does not fail the TU.
    if (( RUNTIME_METRICS )); then publish_sidecar "$sdir" "$rtdir_pub" "$stem" ".cast-runtime.json" "$tag $rel" || true; fi
    if [[ -n "$prev_json" && ! -f "$prev_json" ]]; then
      warn "$rel: prevalence sidecar was not written (expected $prev_json)"
    fi
  else
    status="FAILED (no output)"
    reason="analyzer exited 0 but wrote no usable summary"
  fi

  local line
  line="$(printf '  [%*d/%d] %-6s %s  %s (%ds)' "${#NSRC}" "$idx" "$NSRC" "$tag" "$rel" "$status" $((SECONDS - t0)))"
  if (( TU_OK )); then
    printf '%s\n' "$line"; _log_file "$line"
    if (( nerr > 0 )); then
      warn "$rel: analyzed, but clang reported $nerr error(s); results for this TU may be incomplete"
      hint "first: $first"; hint_for_error "$first"
      hint "full report: $OUT/${stem}.txt (published at the end of the run)"
    fi
    return 0
  fi

  # ---- failure path
  PASS_FAIL=$((PASS_FAIL + 1))
  printf '%s%s%s\n' "$C_RED" "$line" "$C_OFF"; _log_file "$line"
  if [[ -n "$first" ]]; then hint "$first"; hint_for_error "$first"; else hint "last output: $(tail -n 1 "$out_txt")"; fi
  mkdir -p "$OUT/failed"; cp "$out_txt" "$OUT/failed/${stem}.${tag}.txt"
  if [[ "$tag" == c1 ]]; then
    if (( PUBLISH_PARTIAL_C1_SIDECARS )); then
      # Keep whatever the analyzer managed to write, clearly separated from the indexed outputs.
      PARTIAL_ARTIFACTS=()
      publish_partial "$sdir" "$OUT/partial-summaries" "$stem" ".cast-summary.json" summary
      (( REPORTS ))     && publish_partial "$rdir" "$OUT/partial-reports" "$stem" ".cast-report.json" report
      (( PAPER_FACTS )) && publish_partial "$sdir" "$OUT/partial-paper-facts" "$stem" ".cast-paper-facts.json" paper-facts
      (( RUNTIME_METRICS )) && publish_partial "$sdir" "$OUT/partial-runtime-metrics" "$stem" ".cast-runtime.json" runtime-timing
      if [[ -n "$prev_json" && -f "$prev_json" ]]; then
        mkdir -p "$OUT/partial-prevalence-metrics"; mv "$prev_json" "$OUT/partial-prevalence-metrics/"
        PARTIAL_ARTIFACTS+=("prevalence|partial-prevalence-metrics/$(basename "$prev_json")")
      fi
      write_partial_status "$stem" "$src" "$rc" "failed/${stem}.${tag}.txt" "$reason"
      hint "kept ${#PARTIAL_ARTIFACTS[@]} partial sidecar(s); status: $OUT/partial-status/${stem}.cast-analysis-status.json"
    elif [[ -n "$prev_json" ]]; then
      rm -f "$prev_json"    # possibly incomplete: do not mix it with good sidecars
    fi
  fi
  hint "full output: $OUT/failed/${stem}.${tag}.txt (tail also in commands.log)"

  # An import-round failure must not silently drop the TU from the next index:
  # fall back to the previous phase's results and say so, loudly.
  local fallback="none"
  if [[ -n "$prev_sum" && -f "$prev_sum/${stem}.cast-summary.json" ]]; then
    mkdir -p "$sdir_pub"; cp "$prev_sum/${stem}.cast-summary.json" "$sdir_pub/"
    if [[ -f "$prev_text/${stem}.txt" ]]; then
      {
        echo "##############################################################################"
        echo "### STALE REPORT: $tag failed for this TU ($reason)"
        echo "### The content below is the $prev_tag result. It does NOT include cross-TU"
        echo "### import information from this round. Failure output: failed/${stem}.${tag}.txt"
        echo "##############################################################################"
        echo
        cat "$prev_text/${stem}.txt"
      } >"$out_txt.stale" && mv "$out_txt.stale" "$out_txt"
    else
      rm -f "$out_txt"
    fi
    fallback="$prev_tag"
    STALE+=("$tag"$'\t'"$rel")
    loud "STALE RESULT: $rel failed in $tag - FALLING BACK to its $prev_tag summary/report"
    hint "its final results lack the cross-TU information gained in $tag; fix the failure above and re-run"
  else
    rm -f "$out_txt"
  fi
  FAILED+=("$tag"$'\t'"$rc"$'\t'"$src"$'\t'"$reason"$'\t'"$fallback")
  if (( STOP_ON_FAIL )); then STOP_ALL=1; err "stopping the run: --stop-on-fail"; fi
  return 0
}

# run_pass TAG [INDEX_FILE PREV_TAG PREV_SUM_DIR PREV_TEXT_DIR]
# Sets LATEST_TEXT/SUM/REP/PF to the directories this pass produced.
run_pass() {
  local tag="$1" index="${2:-}" prev_tag="${3:-}" prev_sum="${4:-}" prev_text="${5:-}" i skipped=0
  LATEST_TEXT="$WORK/$tag-text"; LATEST_SUM="$WORK/$tag-summaries"
  LATEST_REP="$WORK/$tag-reports"; LATEST_PF="$WORK/$tag-paper-facts"; LATEST_RT="$WORK/$tag-runtime-metrics"
  mkdir -p "$LATEST_TEXT" "$LATEST_SUM"
  PASS_OK=0; PASS_FAIL=0
  for ((i = 0; i < NSRC; i++)); do
    if [[ -n "${SKIP[i]:-}" ]]; then skipped=$((skipped + 1)); continue; fi
    run_tu "$tag" $((i + 1)) "${STEMS[i]}" "${SRCS[i]}" "${RELS[i]}" \
           "$LATEST_TEXT" "$LATEST_SUM" "$LATEST_REP" "$LATEST_PF" "$LATEST_RT" "$index" "$prev_tag" "$prev_sum" "$prev_text"
    [[ "$tag" == c1 && $TU_OK -eq 0 ]] && SKIP[i]=1
    (( STOP_ALL )) && break
  done
  local extra=""; (( skipped > 0 )) && extra=", $skipped skipped (failed in C1)"
  info "  $tag: $PASS_OK ok, $PASS_FAIL failed$extra"
}

run_index() {  # run_index REASON OUTPUT SUMMARY_DIR [extra args...]
  local reason="$1" outfile="$2" sumdir="$3"; shift 3
  local cmd=("$PYTHON" "$BUILD_INDEX" "$sumdir" --project-kind real --build-reason "$reason" "$@" -o "$outfile")
  { printf 'index(%s): ' "$reason"; printf '%q ' "${cmd[@]}"; echo; } >>"$LOG_FILE"
  local out
  if ! out="$("${cmd[@]}" 2>&1)"; then
    { echo "===== index build ($reason) failed ====="; echo "$out"; } >>"$LOG_FILE"
    err "building the project summary index failed ($reason)"
    printf '%s\n' "$out" | tail -n 5 | sed 's/^/         /' >&2
    hint "full output in $LOG_FILE"
    return 1
  fi
  { echo "===== index build ($reason) ====="; echo "$out"; } >>"$LOG_FILE"
  local w p nf ne
  nf="$(sed -n 's/^  summary files: *//p' <<<"$out")"; ne="$(sed -n 's/^  dep edges: *//p' <<<"$out")"
  info "  index ($reason): ${nf:-?} summaries, ${ne:-?} dependency edges"
  w="$(sed -n 's/^  warnings: *//p' <<<"$out")"; p="$(sed -n 's/^  problems: *//p' <<<"$out")"
  [[ "${w:-0}" != 0 ]] && warn "index ($reason): $w warning(s); see commands.log"
  [[ "${p:-0}" != 0 ]] && warn "index ($reason): $p unresolved cross-TU problem(s); see commands.log"
  return 0
}

has_cross_tu_edges() {  # exit 0: import rounds are useful (or unknown) / 1: purely local project
  "$PYTHON" - "$1" <<'PY'
import json, sys
try: edges = json.load(open(sys.argv[1], encoding="utf-8")).get("summaryDependencyEdges")
except Exception: raise SystemExit(0)
if not isinstance(edges, list) or any(not isinstance(e, dict) or "crossTUCandidate" not in e for e in edges): raise SystemExit(0)
raise SystemExit(0 if any(e.get("crossTUCandidate") for e in edges) else 1)
PY
}

# Select the TUs of the current project.  Fills STEMS/SRCS/RELS.
discover_sources() {
  local SRC_TSV="$WORK/sources.tsv"
  "$PYTHON" - "$COMPDB" "$PROJECT_DIR" "$SOURCE_ROOT" "$SRC_TSV" <<'PY'
import json, os, re, sys
compdb, project, source_root, out_tsv = sys.argv[1:5]
W, N = os.environ["CH_WARN"], os.environ["CH_NOTE"]
def warn(m): print(f"{W} {m}", file=sys.stderr)
def note(m): print(f"{N} {m}")
exts = tuple(e.lower() for e in os.environ["CH_EXT"].split())
flt = re.compile(os.environ["CH_FILTER"]) if os.environ["CH_FILTER"] else None
exc = re.compile(os.environ["CH_EXCLUDE"]) if os.environ["CH_EXCLUDE"] else None
try:
    with open(compdb, encoding="utf-8") as fh: entries = json.load(fh)
except (OSError, ValueError) as e:
    print(f"error: cannot read {compdb}: {e}", file=sys.stderr)
    print("         the file must be valid JSON; regenerate it with your build system", file=sys.stderr)
    sys.exit(2)
if not isinstance(entries, list) or not entries:
    print(f"error: {compdb} is empty or not a JSON list of compile commands", file=sys.stderr)
    print("         was the build run with CMAKE_EXPORT_COMPILE_COMMANDS=ON / bear? did it compile anything?", file=sys.stderr)
    sys.exit(2)
cdir = os.path.dirname(os.path.abspath(compdb))
c = dict(entries=len(entries), nofile=0, nocmd=0, ext=0, filt=0, exc=0, missing=0, dup=0, baddir=0)
ex_missing = ex_baddir = None
seen, srcs = set(), []
for e in entries:
    if not isinstance(e, dict) or not e.get("file"): c["nofile"] += 1; continue
    d = e.get("directory") or cdir
    if not (e.get("command") or e.get("arguments")): c["nocmd"] += 1
    f = e["file"] if os.path.isabs(e["file"]) else os.path.join(d, e["file"])
    f = os.path.normpath(os.path.abspath(f))
    if not f.lower().endswith(exts): c["ext"] += 1; continue
    if flt and not flt.search(f): c["filt"] += 1; continue
    if exc and exc.search(f): c["exc"] += 1; continue
    if not os.path.isdir(d): c["baddir"] += 1; ex_baddir = ex_baddir or d
    if not os.path.exists(f): c["missing"] += 1; ex_missing = ex_missing or f; continue
    if f in seen: c["dup"] += 1; continue
    seen.add(f); srcs.append(f)
srcs.sort()
if c["nofile"]: warn(f"{c['nofile']} compile_commands entries have no 'file' field and were ignored")
if c["nocmd"]: warn(f"{c['nocmd']} entries have neither 'command' nor 'arguments': Clang will use default flags for them")
if c["baddir"]:
    warn(f"{c['baddir']} entries have a 'directory' that does not exist (e.g. {ex_baddir})")
    print("         the compile database was probably generated on another machine or inside a container;", file=sys.stderr)
    print("         regenerate it here, or rewrite the paths (sed -i 's#/old/prefix#/new/prefix#g' compile_commands.json)", file=sys.stderr)
if c["missing"]:
    warn(f"{c['missing']} source file(s) listed in compile_commands.json do not exist (e.g. {ex_missing}); skipped")
    if not srcs:
        print("         every source is missing: the paths in the database do not match this machine;", file=sys.stderr)
        print("         regenerate the database or fix the path prefix.", file=sys.stderr)
if not srcs:
    print(f"error: no analyzable sources selected from {c['entries']} compile_commands entries "
          f"(wrong suffix: {c['ext']}, --filter: {c['filt']}, --exclude: {c['exc']}, missing: {c['missing']})", file=sys.stderr)
    print("         relax --filter/--exclude/--extensions or check the compile database", file=sys.stderr)
    sys.exit(3)
under = lambda p, r: p == r or p.startswith(r.rstrip("/") + "/")
if source_root: root = source_root
else:
    root = project
    if not all(under(s, root) for s in srcs):
        try: root = os.path.commonpath(srcs)
        except ValueError: root = "/"
        if len(srcs) == 1 or root in srcs: root = os.path.dirname(root)
        note(f"some sources lie outside the project directory; output names are relative to their common parent {root}")
        note("use --source-root to choose a different base")
outside = [s for s in srcs if not under(s, root)]
if outside: warn(f"{len(outside)} source(s) lie outside the source root {root}; their output names use the full path")
used, rows = {}, []
for s in srcs:
    rel = os.path.relpath(s, root) if under(s, root) else s.lstrip("/")
    stem = re.sub(r"^_|_$", "", re.sub(r"__+", "_", re.sub(r"[^A-Za-z0-9._-]", "_", rel)))
    if stem in used:
        used[stem] += 1
        new = f"{stem}__{used[stem]}"
        warn(f"output-name collision: {rel} -> {new} (two paths sanitize to '{stem}')")
        stem = new
    else: used[stem] = 1
    rows.append((stem, s, rel.replace(os.sep, "/")))
with open(out_tsv, "w", encoding="utf-8") as fh:
    for r in rows: fh.write("\t".join(r) + "\n")
note(f"{len(rows)} translation unit(s) selected from {c['entries']} compile_commands entries"
     f" (skipped: {c['ext']} by suffix, {c['filt']} by --filter, {c['exc']} by --exclude, {c['dup']} duplicate)")
PY
  [[ $? -eq 0 ]] || return 1
  STEMS=(); SRCS=(); RELS=()
  local s p r
  while IFS=$'\t' read -r s p r; do STEMS+=("$s"); SRCS+=("$p"); RELS+=("$r"); done <"$SRC_TSV"
  NSRC=${#STEMS[@]}
}

# Write failed-tus.txt, clean up, print the per-project summary.
# Sets P_STATUS and returns 0 (all good) / 1.
project_finish() {
  local n
  if [[ ${#FAILED[@]} -gt 0 ]]; then
    { printf 'phase\texit\tsource\treason\tfallback\n'; printf '%s\n' "${FAILED[@]}"; } >"$OUT/failed-tus.txt"
  fi
  if (( ! KEEP )); then
    rm -rf "$WORK"; rm -f "$OUT"/import-round-*-index-input.json
  fi
  n=$(find "$OUT/summaries" -name '*.cast-summary.json' 2>/dev/null | wc -l)
  P_TUS=$NSRC; P_SUMS=$n; P_FAILED=${#FAILED[@]}; P_STALE=${#STALE[@]}; P_WARN=$WARN_COUNT
  echo
  info "  ${P_NAME_CUR}: $NSRC TUs, $n final summaries, ${#FAILED[@]} failed, ${#STALE[@]} stale, $WARN_COUNT warnings, $((SECONDS - P_START))s"
  if [[ ${#STALE[@]} -gt 0 ]]; then
    printf '%s!!! %d TU(s) in "%s" have STALE results (import round failed, previous-phase fallback used):%s\n' \
      "$C_RED" "${#STALE[@]}" "$P_NAME_CUR" "$C_OFF" >&2
    printf '%s\n' "${STALE[@]}" | awk -F'\t' '{printf "!!!    %s  (failed in %s)\n", $2, $1}' >&2
  fi
  if [[ ${#FAILED[@]} -gt 0 ]]; then
    err "${#FAILED[@]} analysis step(s) failed in $P_NAME_CUR; see $OUT/failed-tus.txt"
    P_STATUS="FAILED"
    (( ${#STALE[@]} == ${#FAILED[@]} )) && P_STATUS="STALE"
    (( n == 0 )) && P_STATUS="FAILED"
    return 1
  fi
  P_STATUS="OK"; return 0
}

aggregate_runtime() {  # aggregate_runtime SIDECAR_DIR... OUT_DIR LABEL  (last two args are OUT_DIR, LABEL)
  local args=("$@") label out_dir dirs
  label="${args[${#args[@]}-1]}"; out_dir="${args[${#args[@]}-2]}"; dirs=("${args[@]:0:${#args[@]}-2}")
  local cmd=("$PYTHON" "$RUNTIME_AGGREGATOR" "${dirs[@]}"
             --per-tu-csv "$out_dir/runtime-duration-by-tu.csv"
             --per-project-csv "$out_dir/runtime-duration-by-project.csv"
             --latex-out "$out_dir/runtime-duration.tex")
  { printf 'runtime-aggregate(%s): ' "$label"; printf '%q ' "${cmd[@]}"; echo; } >>"${LOG_FILE:-/dev/null}"
  local out
  if out="$("${cmd[@]}" 2>&1)"; then
    info "  runtime timings: $out_dir/runtime-duration-by-project.csv"
  else
    warn "runtime timing aggregation failed for $label"
    printf '%s\n' "$out" | tail -n 3 | sed 's/^/         /' >&2
    hint "the per-TU sidecars are still in ${dirs[0]}"
  fi
}

# --------------------------------------------------------- one project -----
analyze_project() {  # analyze_project INDEX
  local pi="$1" r tag prev_tag prev_sum prev_text n
  PROJECT_DIR="${P_DIR[pi]}"; COMPDB="${P_COMPDB[pi]}"; P_NAME_CUR="${P_NAME[pi]}"
  OUT="$ROOT_OUT/$P_NAME_CUR"; WORK="$OUT/.work"
  FAILED=(); SKIP=(); STALE=(); WARN_COUNT=0; NSRC=0; P_START=$SECONDS
  P_TUS=0; P_SUMS=0; P_FAILED=0; P_STALE=0; P_WARN=0; P_STATUS="SKIPPED"
  LOG_FILE=""
  echo
  step "[$((pi + 1))/$NPROJ] Project: $P_NAME_CUR  ($PROJECT_DIR)"

  mkdir -p "$OUT/summaries" "$WORK"
  LOG_FILE="$OUT/commands.log"; : >"$LOG_FILE"
  (( REPORTS ))     && mkdir -p "$OUT/reports"
  (( PAPER_FACTS )) && mkdir -p "$OUT/paper-facts"
  (( RUNTIME_METRICS )) && mkdir -p "$OUT/runtime-metrics"
  {
    echo "# project: $PROJECT_DIR"
    if command -v git >/dev/null 2>&1; then echo "# project git commit: $(git -C "$PROJECT_DIR" rev-parse HEAD 2>/dev/null || echo n/a)"; fi
  } >>"$LOG_FILE"

  # compile database
  if [[ -z "$COMPDB" ]]; then
    warn "no compile_commands.json for $P_NAME_CUR: synthesizing one in $OUT/synthetic-compdb"
    hint "include paths, macros and generated headers are guesses; prefer a real compile database"
    hint "if the files are independent programs rather than one linked program, add --single-tu"
    if ! n="$(synthesize_compdb "$PROJECT_DIR" "$OUT/synthetic-compdb")"; then
      err "could not synthesize compile_commands.json for $P_NAME_CUR"
      hint "check --extensions / --exclude, or supply a real compile database with --compdb"
      rm -rf "$WORK"; return 1
    fi
    info "synthesized $n compile command(s)"
    COMPDB="$OUT/synthetic-compdb/compile_commands.json"
  fi
  COMPDB_DIR="$(dirname "$COMPDB")"
  info "compile db:     $COMPDB"

  if [[ -n "$SOURCE_ROOT" && ! -d "$SOURCE_ROOT" ]]; then
    err "--source-root is not a directory: $SOURCE_ROOT"; return 1
  fi
  if ! discover_sources; then
    err "skipping $P_NAME_CUR: no usable sources (see messages above)"; rm -rf "$WORK"; return 1
  fi
  _log_file "sources: $NSRC (list in .work/sources.tsv)"
  (( NSRC > 400 )) && info "note: $NSRC TUs; each is analyzed once per pass, so this can take a while"

  # ---- C1
  echo
  step "C1: per-TU analysis ($NSRC TUs)"
  run_pass c1
  local c1_sum="$LATEST_SUM" c1_text="$LATEST_TEXT" c1_rep="$LATEST_REP" c1_pf="$LATEST_PF" c1_rt="$LATEST_RT"
  if (( PASS_OK == 0 )); then
    err "no translation unit of $P_NAME_CUR was analyzed successfully"
    hint "see the first failure above; most often it is a missing --resource-dir, a stale compile_commands.json,"
    hint "or missing generated headers (run the project's configure/build once)"
    project_finish; P_STATUS="FAILED"; return 1
  fi
  if (( PASS_FAIL > 0 )); then
    warn "$PASS_FAIL of $NSRC TUs failed in C1; they are missing from the index, so calls into them stay unresolved"
    (( PASS_FAIL * 2 > NSRC )) && warn "more than half of the TUs failed: the setup (flags, headers, resource dir) is probably wrong"
  fi

  local last_text="$c1_text" last_sum="$c1_sum" last_rep="$c1_rep" last_pf="$c1_pf" last_rt="$c1_rt" done_rounds=0
  local index="$OUT/project-summary-index.json"

  if (( STOP_ALL )); then :
  elif (( SINGLE_TU )); then
    info "  --single-tu: skipping index and import rounds"
  else
    # ---- C2
    echo
    step "C2: build project summary index"
    local rounds="$ROUNDS"
    if ! run_index c1-raw "$index" "$c1_sum"; then
      hint "cannot continue without an index; publishing C1 results only"
      rounds=0
    fi

    # ---- import rounds
    if (( rounds > 0 )); then
      if has_cross_tu_edges "$index"; then
        prev_tag=c1; prev_sum="$c1_sum"; prev_text="$c1_text"
        for ((r = 1; r <= rounds; r++)); do
          tag="$(printf 'imp%02d' "$r")"
          echo
          step "Import round $r/$rounds"
          # The index must sit in $OUT: its summaryFile entries are relative to its own directory.
          cp "$index" "$OUT/import-round-$(printf '%02d' "$r")-index-input.json"
          run_pass "$tag" "$OUT/import-round-$(printf '%02d' "$r")-index-input.json" "$prev_tag" "$prev_sum" "$prev_text"
          last_text="$LATEST_TEXT"; last_sum="$LATEST_SUM"; last_rep="$LATEST_REP"; last_pf="$LATEST_PF"; last_rt="$LATEST_RT"
          done_rounds=$r; prev_tag="$tag"; prev_sum="$LATEST_SUM"; prev_text="$LATEST_TEXT"
          (( STOP_ALL )) && break
          if ! compgen -G "$LATEST_SUM/*.cast-summary.json" >/dev/null; then
            err "round $r produced no summaries; stopping refinement"; break
          fi
          run_index "$tag-import-aware" "$index" "$LATEST_SUM" --reuse-output-if-input-unchanged "$index" || break
        done
      else
        echo
        step "Import rounds skipped: no cross-TU call edges in the index"
        info "  running one local-index pass so the final reports use the solved summaries"
        run_pass local-overlay "$index" c1 "$c1_sum" "$c1_text"
        last_text="$LATEST_TEXT"; last_sum="$LATEST_SUM"; last_rep="$LATEST_REP"; last_pf="$LATEST_PF"; last_rt="$LATEST_RT"
      fi
    fi
  fi

  # ---- publish
  echo
  step "Publish final outputs"
  local i missing_text=0
  for ((i = 0; i < NSRC; i++)); do
    [[ -n "${SKIP[i]:-}" ]] && continue
    if [[ -f "$last_text/${STEMS[i]}.txt" ]]; then
      cp "$last_text/${STEMS[i]}.txt" "$OUT/${STEMS[i]}.txt"
    else
      missing_text=$((missing_text + 1))
    fi
  done
  (( missing_text > 0 )) && warn "$missing_text TU(s) have no final text report; see failed-tus.txt"

  compgen -G "$last_sum/*.cast-summary.json" >/dev/null && cp "$last_sum"/*.cast-summary.json "$OUT/summaries/"
  (( REPORTS ))     && compgen -G "$last_rep/*.cast-report.json" >/dev/null && cp "$last_rep"/*.cast-report.json "$OUT/reports/"
  (( PAPER_FACTS )) && compgen -G "$last_pf/*.cast-paper-facts.json" >/dev/null && cp "$last_pf"/*.cast-paper-facts.json "$OUT/paper-facts/"

  if (( RUNTIME_METRICS )); then
    compgen -G "$last_rt/*.cast-runtime.json" >/dev/null && cp "$last_rt"/*.cast-runtime.json "$OUT/runtime-metrics/"
    if compgen -G "$OUT/runtime-metrics/*.cast-runtime.json" >/dev/null; then
      aggregate_runtime "$OUT/runtime-metrics" "$OUT" "$P_NAME_CUR"
    else
      warn "no runtime timing sidecars were produced for $P_NAME_CUR"
    fi
  fi
  if (( PREVALENCE_METRICS )); then
    n=$(find "$OUT/prevalence-metrics" -name '*.cast-prevalence.json' 2>/dev/null | wc -l)
    info "  prevalence sidecars: $n"
    (( n < NSRC - ${#FAILED[@]} )) && warn "only $n prevalence sidecar(s) for $((NSRC - ${#FAILED[@]})) analyzed TU(s)"
  fi

  # Rebuild the index over the *published* summaries so its summaryFile paths
  # stay valid after the work directory is removed.
  if (( ! SINGLE_TU )) && compgen -G "$OUT/summaries/*.cast-summary.json" >/dev/null; then
    run_index final "$index" "$OUT/summaries" || true
  fi
  (( SINGLE_TU )) || info "  import rounds completed: $done_rounds"
  project_finish
}

# ------------------------------------------------------------------- main ---
RUN_START=$SECONDS
RESULTS=()
overall=0
for ((pi = 0; pi < NPROJ; pi++)); do
  analyze_project "$pi" || overall=1
  RESULTS+=("${P_NAME[pi]}"$'\t'"$P_STATUS"$'\t'"$P_TUS"$'\t'"$P_SUMS"$'\t'"$P_FAILED"$'\t'"$P_STALE"$'\t'"$P_WARN"$'\t'"${P_DIR[pi]}")
  LOG_FILE=""
  if (( STOP_ALL )); then warn "run stopped early; remaining projects were not analyzed"; overall=1; break; fi
done

if (( RUNTIME_METRICS && NPROJ > 1 )); then
  mapfile -t rt_dirs < <(find "$ROOT_OUT" -mindepth 2 -maxdepth 2 -type d -name runtime-metrics | sort)
  LOG_FILE=""
  [[ ${#rt_dirs[@]} -gt 0 ]] && aggregate_runtime "${rt_dirs[@]}" "$ROOT_OUT" "all projects"
fi

{ printf 'project\tstatus\ttus\tsummaries\tfailed\tstale\twarnings\tpath\n'; printf '%s\n' "${RESULTS[@]}"; } >"$ROOT_OUT/projects-summary.tsv"

echo
step "Run summary"
printf '  %-28s %-8s %5s %9s %6s %6s\n' project status TUs summaries failed stale
for row in "${RESULTS[@]}"; do
  IFS=$'\t' read -r n st tus sums fl stl wn path <<<"$row"
  col="$C_GRN"; [[ "$st" == OK ]] || col="$C_RED"
  printf '  %-28s %s%-8s%s %5s %9s %6s %6s\n' "$n" "$col" "$st" "$C_OFF" "$tus" "$sums" "$fl" "$stl"
done
info "  warnings: $TOTAL_WARN   elapsed: $((SECONDS - RUN_START))s"
info "  output:   $ROOT_OUT  (table: projects-summary.tsv, log: run.log)"
if (( overall == 0 )); then
  ok "Done: cast history written to $ROOT_OUT"
else
  err "finished with problems: see the failed/stale/skipped projects above"
fi
exit "$overall"

#!/usr/bin/env bash
# cast_history_project.sh - build the cast history of ONE C/C++ project
# (in-tree or out-of-tree) into a chosen output directory.
#
# Clean replacement for run_all_multitu_projects-rp.sh when the benchmark
# lives outside this repository.  It runs the same workflow:
#
#   C1  analyze every translation unit (TU)  -> per-TU cast-summary.json
#   C2  build project-summary-index.json     <- all C1 summaries
#   IMP N import-aware rounds: re-analyze each TU with --enable-summary-import,
#       rebuild the index from the new summaries (default N=2)
#   FIN publish final text + summaries + index to the output directory
#
# Usage:
#   cast_history_project.sh [options] [PROJECT_DIR | path/to/compile_commands.json]
#
#   PROJECT_DIR defaults to the current directory.
#
# Required inputs (everything else is auto-detected; see --help):
#   1. cast-history-tool binary   --tool, $TOOL, ./cast-history-tool,
#                                 ./build/ or <repo>/build/
#   2. compile_commands.json      --compdb, or PROJECT_DIR/{,build/}compile_commands.json,
#                                 or --synthesize-compdb for self-contained sources
#   3. a clang whose builtin headers match the tool's LLVM (stddef.h, ...)
#                                 --resource-dir / --clang, or `clang` on PATH
#   4. python3 + scripts/build_summary_index.py (found next to this script)
#
# Exit status: 0 all TUs analyzed, 1 finished but some TU/phase failed,
#              2 usage or environment problem (nothing was analyzed).

set -uo pipefail

if (( BASH_VERSINFO[0] < 4 )); then
  echo "error: bash >= 4 is required (found ${BASH_VERSION})." >&2
  echo "       on macOS: 'brew install bash' and run with /opt/homebrew/bin/bash." >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SCRIPT_NAME="$(basename "$0")"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"

# ---------------------------------------------------------------- logging ---
if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
  C_RED=$'\033[31m'; C_YEL=$'\033[33m'; C_GRN=$'\033[32m'; C_BLD=$'\033[1m'; C_OFF=$'\033[0m'
else
  C_RED=""; C_YEL=""; C_GRN=""; C_BLD=""; C_OFF=""
fi
LOG_FILE=""
WARN_COUNT=0

_log_file() { [[ -n "$LOG_FILE" ]] && printf '%s\n' "$*" >>"$LOG_FILE"; return 0; }
info()  { printf '%s\n' "$*"; _log_file "$*"; }
step()  { printf '%s==> %s%s\n' "$C_BLD" "$*" "$C_OFF"; _log_file "==> $*"; }
ok()    { printf '%s%s%s\n' "$C_GRN" "$*" "$C_OFF"; _log_file "$*"; }
warn()  { WARN_COUNT=$((WARN_COUNT + 1)); printf '%swarning:%s %s\n' "$C_YEL" "$C_OFF" "$*" >&2; _log_file "warning: $*"; }
hint()  { printf '         %s\n' "$*" >&2; _log_file "         $*"; }
err()   { printf '%serror:%s %s\n' "$C_RED" "$C_OFF" "$*" >&2; _log_file "error: $*"; }
die()   { err "$1"; shift; local h; for h in "$@"; do hint "$h"; done; exit 2; }

# ------------------------------------------------------------------ usage ---
usage() {
  cat <<EOF
Usage: $SCRIPT_NAME [options] [PROJECT_DIR | compile_commands.json]

Build the cast history of the project in PROJECT_DIR (default: current dir)
and write it to the output directory.

Output:
  -o, --output DIR        output directory (default: ./cast-history-out)
      --force             allow OUTPUT to be non-empty and not created by this script;
                          its CONTENTS ARE DELETED
Inputs:
      --compdb PATH       compile_commands.json (or the directory holding it)
      --synthesize-compdb generate a simple compile_commands.json from a recursive
                          source scan when the project has none (flags are guessed:
                          see --std / --cflags; header paths/macros may be wrong)
      --std STD           -std= for synthesized entries (default: c11 / c++17)
      --cflags "FLAGS"    extra flags for synthesized entries, e.g. "-Iinclude -DNDEBUG"
      --source-root DIR   directory that output file names are made relative to
                          (default: PROJECT_DIR, or the common parent of the sources)
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
      --rounds N          import/index refinement rounds (default 2; 0 = C1 only)
      --config-id ID      value for --summary-config-id
      --reports           also emit verbose *.cast-report.json sidecars
      --paper-facts       also emit compact *.cast-paper-facts.json sidecars
      --stop-on-fail      stop at the first TU that fails
      --keep-intermediates keep per-phase work files under OUTPUT/.work
  -h, --help              this help

Output layout:
  OUTPUT/<stem>.txt                    final human-readable report per TU
  OUTPUT/summaries/<stem>.cast-summary.json   final machine-readable summaries
  OUTPUT/project-summary-index.json    index over the final summaries
  OUTPUT/commands.log                  every command + failure output + warnings
  OUTPUT/failed-tus.txt                only if something failed: phase, exit code, source
  OUTPUT/failed/<stem>.<phase>.txt     analyzer output of each failed TU
  (a TU that fails in C1 is left out of the index and not retried in later rounds)
  <stem> is the source path relative to the source root with '/' replaced by '_'.

Examples:
  # project already has build/compile_commands.json
  $SCRIPT_NAME -o /data/out/lua /data/benchmarks/lua-5.4.1

  # run from inside the project, skip tests/vendored code
  cd ~/src/zlib && $SCRIPT_NAME --exclude '/(test|contrib)/' -o ~/out/zlib
EOF
}

# ------------------------------------------------------------- arguments ---
PROJECT_ARG=""
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
ROUNDS=2
CONFIG_ID=""
REPORTS=0
PAPER_FACTS=0
STOP_ON_FAIL=0
KEEP=0

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
    --config-id)          need_value "$@"; CONFIG_ID="$2"; shift 2 ;;
    --reports)            REPORTS=1; shift ;;
    --paper-facts)        PAPER_FACTS=1; shift ;;
    --stop-on-fail)       STOP_ON_FAIL=1; shift ;;
    --keep-intermediates) KEEP=1; shift ;;
    -*)  die "unknown option: $1" "run '$SCRIPT_NAME --help' for the list of options" ;;
    *)
      [[ -z "$PROJECT_ARG" ]] || die "more than one project given: '$PROJECT_ARG' and '$1'" \
        "this script handles one project per run; loop over projects in your shell"
      PROJECT_ARG="$1"; shift ;;
  esac
done

[[ "$ROUNDS" =~ ^[0-9]+$ ]] || die "--rounds must be a non-negative integer, got '$ROUNDS'"

# ------------------------------------------------------------- pre-flight ---
step "Pre-flight checks"

command -v "$PYTHON" >/dev/null 2>&1 || die "python interpreter not found: $PYTHON" \
  "install python3 or pass --python /path/to/python3"

# Project directory (or a compile_commands.json given directly).
PROJECT_ARG="${PROJECT_ARG:-.}"
if [[ -f "$PROJECT_ARG" ]]; then
  [[ "$(basename "$PROJECT_ARG")" == "compile_commands.json" ]] || die \
    "project argument is a file but not compile_commands.json: $PROJECT_ARG" \
    "pass the project directory, or a file named exactly compile_commands.json"
  COMPDB_ARG="${COMPDB_ARG:-$PROJECT_ARG}"
  PROJECT_ARG="$(dirname "$PROJECT_ARG")"
fi
[[ -d "$PROJECT_ARG" ]] || die "project directory not found: $PROJECT_ARG" \
  "check the path; relative paths are resolved from $(pwd -P)"
PROJECT_DIR="$(cd "$PROJECT_ARG" && pwd -P)"
info "project:        $PROJECT_DIR"

# Tool binary.
if [[ -z "$TOOL" ]]; then
  for cand in ./cast-history-tool ./build/cast-history-tool "$REPO_ROOT/build/cast-history-tool"; do
    if [[ -x "$cand" && -f "$cand" ]]; then TOOL="$cand"; break; fi
  done
fi
[[ -n "$TOOL" ]] || die "cast-history-tool binary not found" \
  "build it:  cmake -S $REPO_ROOT -B $REPO_ROOT/build && cmake --build $REPO_ROOT/build -j" \
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
info "tool:           $TOOL"

# Index builder.
BUILD_INDEX="${BUILD_INDEX:-$SCRIPT_DIR/build_summary_index.py}"
[[ -f "$BUILD_INDEX" ]] || die "index builder not found: $BUILD_INDEX" \
  "it normally sits next to this script; otherwise pass --build-index /path/to/build_summary_index.py"
info "index builder:  $BUILD_INDEX"

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
      for c in clang clang-20 clang-19 clang-18 clang-17 clang-16 clang-15 clang-14; do
        if command -v "$c" >/dev/null 2>&1; then CLANG_BIN="$c"; break; fi
      done
    fi
    [[ -n "$CLANG_BIN" ]] && command -v "$CLANG_BIN" >/dev/null 2>&1 || die \
      "no clang found to locate the builtin headers (${CLANG_BIN:-searched clang, clang-14..20})" \
      "install clang, or pass --clang /path/to/clang (same LLVM version as the tool)" \
      "or pass --resource-dir <dir containing include/stddef.h>"
    RESOURCE_DIR="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)" || die \
      "'$CLANG_BIN -print-resource-dir' failed" "pass --resource-dir explicitly"
    info "clang:          $CLANG_BIN ($("$CLANG_BIN" --version 2>/dev/null | head -1))"
  fi
  [[ -f "$RESOURCE_DIR/include/stddef.h" ]] || die \
    "invalid clang resource directory: $RESOURCE_DIR" \
    "expected builtin header $RESOURCE_DIR/include/stddef.h" \
    "use the resource dir of the clang matching the LLVM the tool was built with (--resource-dir / --clang)"
  RESOURCE_ARGS=("--extra-arg-before=-resource-dir=$RESOURCE_DIR")
  info "resource dir:   $RESOURCE_DIR"
fi

# Output directory.  We only ever delete a directory this script created
# (marker file) or one the user explicitly allowed with --force.
mkdir -p "$OUTPUT_DIR" 2>/dev/null || die "cannot create output directory: $OUTPUT_DIR" \
  "check permissions and that the parent exists"
[[ -w "$OUTPUT_DIR" ]] || die "output directory is not writable: $OUTPUT_DIR" "check permissions"
OUT="$(cd "$OUTPUT_DIR" && pwd -P)"
MARKER="$OUT/.cast-history-output"
if [[ "$OUT" == "/" || "$OUT" == "${HOME:-/nonexistent}" || "$OUT" == "$PROJECT_DIR" || "$PROJECT_DIR/" == "$OUT/"* ]]; then
  die "refusing to use $OUT as output: it is, or contains, the project/home/root directory" \
    "choose a dedicated directory, e.g. -o $OUT/cast-history-out"
fi
if [[ -n "$(ls -A "$OUT")" && ! -f "$MARKER" ]]; then
  (( FORCE )) || die "output directory is not empty and was not created by this script: $OUT" \
    "choose a new/empty directory, or pass --force to overwrite its contents"
fi
find "$OUT" -mindepth 1 -delete 2>/dev/null
echo "created by $SCRIPT_NAME on $(date -u +%FT%TZ)" >"$MARKER"
LOG_FILE="$OUT/commands.log"
: >"$LOG_FILE"
WORK="$OUT/.work"
mkdir -p "$WORK" "$OUT/summaries"
(( REPORTS ))    && mkdir -p "$OUT/reports"
(( PAPER_FACTS )) && mkdir -p "$OUT/paper-facts"
info "output:         $OUT"

{
  echo "# $SCRIPT_NAME  $(date -u +%FT%TZ)"
  printf '# argv:'; printf ' %q' ${ORIG_ARGS[@]+"${ORIG_ARGS[@]}"}; echo
  echo "# cwd: $(pwd -P)"
  echo "# project: $PROJECT_DIR"
  echo "# tool: $TOOL"
  command -v git >/dev/null 2>&1 && {
    echo "# project git commit: $(git -C "$PROJECT_DIR" rev-parse HEAD 2>/dev/null || echo n/a)"
    echo "# tool-repo git commit: $(git -C "$REPO_ROOT" rev-parse HEAD 2>/dev/null || echo n/a)"
  }
} >>"$LOG_FILE"

# ------------------------------------------------- compile_commands.json ---
sanitize_stem() { printf '%s' "$1" | sed -e 's/[^A-Za-z0-9._-]/_/g' -e 's/__*/_/g' -e 's/^_//' -e 's/_$//'; }

# Prefixes the embedded Python uses for diagnostics (keeps colors consistent).
export CH_WARN="${C_YEL}warning:${C_OFF}" CH_NOTE="note:"
export CH_EXT="$SRC_EXT" CH_FILTER="$SRC_FILTER" CH_EXCLUDE="$SRC_EXCLUDE"

synthesize_compdb() {  # synthesize_compdb OUT_DIR
  local dir="$1"; mkdir -p "$dir"
  "$PYTHON" - "$dir/compile_commands.json" "$PROJECT_DIR" "$SYNTH_STD" "$SYNTH_CFLAGS" <<'PY'
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

find_compdb() {
  local cand
  if [[ -n "$COMPDB_ARG" ]]; then
    if [[ -d "$COMPDB_ARG" ]]; then cand="$COMPDB_ARG/compile_commands.json"; else cand="$COMPDB_ARG"; fi
    [[ -f "$cand" ]] || die "compile_commands.json not found at: $COMPDB_ARG" \
      "pass the file or the directory that contains it"
    [[ "$(basename "$cand")" == "compile_commands.json" ]] || die \
      "the tool only reads files named compile_commands.json, got: $(basename "$cand")" \
      "copy or symlink it:  ln -s $(basename "$cand") $(dirname "$cand")/compile_commands.json"
    COMPDB="$(cd "$(dirname "$cand")" && pwd -P)/compile_commands.json"; return
  fi
  for cand in "$PROJECT_DIR/compile_commands.json" "$PROJECT_DIR/build/compile_commands.json"; do
    [[ -f "$cand" ]] && { COMPDB="$cand"; return; }
  done
  local found=()
  mapfile -t found < <(find "$PROJECT_DIR" -mindepth 2 -maxdepth 2 -name compile_commands.json 2>/dev/null | sort)
  if [[ ${#found[@]} -eq 1 ]]; then
    COMPDB="${found[0]}"; info "note: using ${found[0]} (only compile_commands.json one level down)"; return
  elif [[ ${#found[@]} -gt 1 ]]; then
    err "several compile_commands.json files found under $PROJECT_DIR:"
    printf '         %s\n' "${found[@]}" >&2
    hint "pick one with --compdb <file-or-dir>"
    exit 2
  fi
  if (( SYNTH )); then
    warn "no compile_commands.json found: synthesizing one in $OUT/synthetic-compdb"
    hint "include paths, macros and generated headers are guesses; prefer a real compile database"
    local n
    n="$(synthesize_compdb "$OUT/synthetic-compdb")" || die "could not synthesize compile_commands.json" \
      "check --extensions / --exclude, or supply a real compile database with --compdb"
    info "synthesized $n compile command(s)"
    COMPDB="$OUT/synthetic-compdb/compile_commands.json"; return
  fi
  err "no compile_commands.json found for project: $PROJECT_DIR"
  hint "looked in: $PROJECT_DIR/ and $PROJECT_DIR/build/ (and one level below)"
  hint "create one, then re-run (or pass --compdb):"
  hint "  CMake:      cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
  hint "  make/other: bear -- make        (or: compiledb -n make)"
  hint "  self-contained sources only: re-run with --synthesize-compdb"
  exit 2
}

COMPDB=""
find_compdb
COMPDB_DIR="$(dirname "$COMPDB")"
info "compile db:     $COMPDB"

# ------------------------------------------------------ source discovery ---
if [[ -n "$SOURCE_ROOT" ]]; then
  [[ -d "$SOURCE_ROOT" ]] || die "--source-root is not a directory: $SOURCE_ROOT"
  SOURCE_ROOT="$(cd "$SOURCE_ROOT" && pwd -P)"
fi
export CH_REQUIRE_EXISTS=1

SRC_TSV="$WORK/sources.tsv"
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
rc=$?
case $rc in
  0) ;;
  *) exit 2 ;;
esac

STEMS=(); SRCS=(); RELS=()
while IFS=$'\t' read -r s p r; do STEMS+=("$s"); SRCS+=("$p"); RELS+=("$r"); done <"$SRC_TSV"
NSRC=${#STEMS[@]}
_log_file "sources: $NSRC (list in .work/sources.tsv)"
(( NSRC > 400 )) && info "note: $NSRC TUs and $((ROUNDS + 2)) passes; this can take a while (each TU is analyzed once per pass)"

# ----------------------------------------------------------- run helpers ---
FAILED=()      # "phase<TAB>exit<TAB>source<TAB>reason"
SKIP=()        # SKIP[i]=1: TU failed in C1, so later passes do not retry it
PASS_OK=0; PASS_FAIL=0; TU_OK=1

# Suggest a fix for the first clang error we saw.
hint_for_error() {
  local msg="$1"
  case "$msg" in
    *"'std"*"h' file not found"*|*"'stdarg.h' file not found"*|*"'stdbool.h' file not found"*|*"'stdint.h' file not found"*|*"'limits.h' file not found"*)
      hint "builtin clang header missing: --resource-dir ($RESOURCE_DIR) does not match the clang the tool was built with"
      hint "pass --resource-dir \$(clang-NN -print-resource-dir) for the right LLVM version" ;;
    *"file not found"*)
      hint "missing include: check the -I flags in compile_commands.json for this TU;"
      hint "generated headers (config.h, ...) exist only after the project's configure/build step has run." ;;
    *"unknown type name"*|*"undeclared identifier"*)
      hint "likely a missing -D macro or include path in compile_commands.json for this TU." ;;
  esac
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

# run_tu TAG IDX STEM SRC REL TEXT_DIR SUM_DIR REP_DIR PF_DIR [INDEX_FILE] [PREV_SUM_DIR]
# Analyze one TU, publish its outputs, print one progress line.
run_tu() {
  local tag="$1" idx="$2" stem="$3" src="$4" rel="$5" tdir="$6" sdir_pub="$7" rdir_pub="$8" pdir_pub="$9"
  local index="${10:-}" prev_sum="${11:-}"
  local tmp="$WORK/$tag-tmp/$stem" sdir rdir out_txt rc t0 nerr first status="ok"
  sdir="$tmp/s"; rdir="$tmp/r"; out_txt="$tdir/${stem}.txt"
  rm -rf "$tmp"; mkdir -p "$sdir" "$tdir"
  local cmd=("$TOOL")
  [[ -n "$index" ]] && cmd+=(--enable-summary-import --summary-index "$index")
  cmd+=(--summary-dir "$sdir")
  if (( REPORTS )); then mkdir -p "$rdir"; cmd+=(--summary-report-dir "$rdir"); fi
  (( PAPER_FACTS )) && cmd+=(--paper-facts-dir "$sdir")
  [[ -n "$CONFIG_ID" ]] && cmd+=(--summary-config-id "$CONFIG_ID-$tag")
  cmd+=(${RESOURCE_ARGS[@]+"${RESOURCE_ARGS[@]}"} -p "$COMPDB_DIR" "$src")

  { printf '%s: ' "$tag"; printf '%q ' "${cmd[@]}"; echo; } >>"$LOG_FILE"
  t0=$SECONDS
  { echo "Command:"; printf '%q ' "${cmd[@]}"; echo; echo; echo "Output:"; "${cmd[@]}"; } >"$out_txt" 2>&1
  rc=$?
  nerr="$(grep -E -c ':[0-9]+:[0-9]+: (fatal )?error:' "$out_txt")"
  first="$(grep -E -m1 ':[0-9]+:[0-9]+: (fatal )?error:' "$out_txt" | sed -e "s#$PROJECT_DIR/##")"

  TU_OK=0
  if (( rc != 0 )); then
    status="FAILED (exit $rc)"
    PASS_FAIL=$((PASS_FAIL + 1))
    FAILED+=("$tag"$'\t'"$rc"$'\t'"$src"$'\t'"${first:-analyzer exited nonzero}")
    { echo; echo "===== $tag failure output for $rel ====="; tail -n 40 "$out_txt"; } >>"$LOG_FILE"
  else
    if publish_sidecar "$sdir" "$sdir_pub" "$stem" ".cast-summary.json" "$tag $rel" \
       && { (( ! REPORTS )) || publish_sidecar "$rdir" "$rdir_pub" "$stem" ".cast-report.json" "$tag $rel"; } \
       && { (( ! PAPER_FACTS )) || publish_sidecar "$sdir" "$pdir_pub" "$stem" ".cast-paper-facts.json" "$tag $rel"; }; then
      PASS_OK=$((PASS_OK + 1)); TU_OK=1
    else
      status="FAILED (no output)"
      PASS_FAIL=$((PASS_FAIL + 1))
      FAILED+=("$tag"$'\t'"0"$'\t'"$src"$'\t'"analyzer exited 0 but wrote no usable summary")
    fi
  fi

  local line
  line="$(printf '  [%*d/%d] %-5s %s  %s (%ds)' "${#NSRC}" "$idx" "$NSRC" "$tag" "$rel" "$status" $((SECONDS - t0)))"
  if [[ "$status" == ok ]]; then
    if (( nerr > 0 )); then
      printf '%s\n' "$line"; _log_file "$line"
      warn "$rel: analyzed, but clang reported $nerr error(s); results for this TU may be incomplete"
      hint "first: $first"; hint_for_error "$first"
      hint "full report: $OUT/${stem}.txt (published at the end of the run)"
    else
      printf '%s\n' "$line"; _log_file "$line"
    fi
  else
    printf '%s%s%s\n' "$C_RED" "$line" "$C_OFF"; _log_file "$line"
    if [[ -n "$first" ]]; then
      hint "$first"; hint_for_error "$first"
    else
      hint "last output: $(tail -n 1 "$out_txt")"
    fi
    mkdir -p "$OUT/failed"; cp "$out_txt" "$OUT/failed/${stem}.${tag}.txt"
    hint "full output: $OUT/failed/${stem}.${tag}.txt (tail also in commands.log)"
    # An import-round failure must not silently drop the TU from the next index.
    if [[ -n "$prev_sum" && -f "$prev_sum/${stem}.cast-summary.json" ]]; then
      mkdir -p "$sdir_pub"; cp "$prev_sum/${stem}.cast-summary.json" "$sdir_pub/"
      warn "$rel: keeping its previous-phase summary so the index stays complete (its $tag text report is missing)"
    fi
    (( STOP_ON_FAIL )) && { err "stopping: --stop-on-fail"; finish 1; }
  fi
}

# run_pass TAG [INDEX_FILE] [PREV_SUM_DIR]
# Sets LATEST_TEXT/SUM/REP/PF to the directories this pass produced.
run_pass() {
  local tag="$1" index="${2:-}" prev="${3:-}" i
  LATEST_TEXT="$WORK/$tag-text"; LATEST_SUM="$WORK/$tag-summaries"
  LATEST_REP="$WORK/$tag-reports"; LATEST_PF="$WORK/$tag-paper-facts"
  mkdir -p "$LATEST_TEXT" "$LATEST_SUM"
  PASS_OK=0; PASS_FAIL=0
  local skipped=0
  for ((i = 0; i < NSRC; i++)); do
    if [[ -n "${SKIP[i]:-}" ]]; then skipped=$((skipped + 1)); continue; fi
    run_tu "$tag" $((i + 1)) "${STEMS[i]}" "${SRCS[i]}" "${RELS[i]}" \
           "$LATEST_TEXT" "$LATEST_SUM" "$LATEST_REP" "$LATEST_PF" "$index" "$prev"
    [[ "$tag" == c1 && $TU_OK -eq 0 ]] && SKIP[i]=1
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
  # Surface index-level problems that would otherwise hide in the log.
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

# ----------------------------------------------------------------- finish ---
START=$SECONDS
finish() {
  local code="${1:-0}" n
  if [[ ${#FAILED[@]} -gt 0 ]]; then
    { printf 'phase\texit\tsource\treason\n'; printf '%s\n' "${FAILED[@]}"; } >"$OUT/failed-tus.txt"
  fi
  if (( ! KEEP )); then
    rm -rf "$WORK"; rm -f "$OUT"/import-round-*-index-input.json
  fi
  n=$(find "$OUT/summaries" -name '*.cast-summary.json' 2>/dev/null | wc -l)
  echo
  step "Summary"
  info "  translation units:   $NSRC"
  info "  final summaries:     $n"
  info "  failed analyses:     ${#FAILED[@]}"
  info "  warnings:            $WARN_COUNT"
  info "  elapsed:             $((SECONDS - START))s"
  info "  output:              $OUT"
  if [[ ${#FAILED[@]} -gt 0 ]]; then
    err "${#FAILED[@]} analysis step(s) failed; details in $OUT/failed-tus.txt and $OUT/commands.log"
    (( code == 0 )) && code=1
  fi
  if (( code == 0 )); then ok "Done: cast history written to $OUT"; fi
  exit "$code"
}
trap 'echo; err "interrupted"; hint "partial results in $OUT"; exit 130' INT TERM

# ------------------------------------------------------------- C1 and C2 ---
echo
step "C1: per-TU analysis ($NSRC TUs)"
run_pass c1
C1_SUM="$LATEST_SUM"; C1_TEXT="$LATEST_TEXT"; C1_REP="$LATEST_REP"; C1_PF="$LATEST_PF"
if (( PASS_OK == 0 )); then
  err "no translation unit was analyzed successfully"
  hint "see the first failure above; most often it is a missing --resource-dir, a stale compile_commands.json,"
  hint "or missing generated headers (run the project's configure/build once)"
  finish 1
fi
if (( PASS_FAIL > 0 )); then
  warn "$PASS_FAIL of $NSRC TUs failed in C1; they are missing from the index, so calls into them stay unresolved"
  (( PASS_FAIL * 2 > NSRC )) && warn "more than half of the TUs failed: the setup (flags, headers, resource dir) is probably wrong"
fi

INDEX="$OUT/project-summary-index.json"
echo
step "C2: build project summary index"
if ! run_index c1-raw "$INDEX" "$C1_SUM"; then
  hint "cannot continue without an index; publishing C1 results only"
  ROUNDS=0
fi

LAST_TEXT="$C1_TEXT"; LAST_SUM="$C1_SUM"; LAST_REP="$C1_REP"; LAST_PF="$C1_PF"
DONE_ROUNDS=0

# ---------------------------------------------------------- import rounds ---
if (( ROUNDS > 0 )); then
  if has_cross_tu_edges "$INDEX"; then
    prev_sum="$C1_SUM"
    for ((r = 1; r <= ROUNDS; r++)); do
      tag="$(printf 'imp%02d' "$r")"
      echo
      step "Import round $r/$ROUNDS"
      # The index must sit in $OUT: its summaryFile entries are relative to its own directory.
      cp "$INDEX" "$OUT/import-round-$(printf '%02d' "$r")-index-input.json"
      run_pass "$tag" "$OUT/import-round-$(printf '%02d' "$r")-index-input.json" "$prev_sum"
      LAST_TEXT="$LATEST_TEXT"; LAST_SUM="$LATEST_SUM"; LAST_REP="$LATEST_REP"; LAST_PF="$LATEST_PF"
      DONE_ROUNDS=$r; prev_sum="$LATEST_SUM"
      if ! compgen -G "$LATEST_SUM/*.cast-summary.json" >/dev/null; then
        err "round $r produced no summaries; stopping refinement"; break
      fi
      run_index "$tag-import-aware" "$INDEX" "$LATEST_SUM" --reuse-output-if-input-unchanged "$INDEX" || break
    done
  else
    echo
    step "Import rounds skipped: no cross-TU call edges in the index"
    info "  running one local-index pass so the final reports use the solved summaries"
    run_pass local-overlay "$INDEX" "$C1_SUM"
    LAST_TEXT="$LATEST_TEXT"; LAST_SUM="$LATEST_SUM"; LAST_REP="$LATEST_REP"; LAST_PF="$LATEST_PF"
  fi
fi

# ----------------------------------------------------------------- publish ---
echo
step "Publish final outputs"
missing_text=0
for ((i = 0; i < NSRC; i++)); do
  [[ -n "${SKIP[i]:-}" ]] && continue
  if [[ -f "$LAST_TEXT/${STEMS[i]}.txt" ]]; then
    cp "$LAST_TEXT/${STEMS[i]}.txt" "$OUT/${STEMS[i]}.txt"
  else
    missing_text=$((missing_text + 1))
  fi
done
(( missing_text > 0 )) && warn "$missing_text TU(s) have no final text report (they failed in an import round); see failed-tus.txt"

compgen -G "$LAST_SUM/*.cast-summary.json" >/dev/null && cp "$LAST_SUM"/*.cast-summary.json "$OUT/summaries/"
(( REPORTS ))     && compgen -G "$LAST_REP/*.cast-report.json" >/dev/null && cp "$LAST_REP"/*.cast-report.json "$OUT/reports/"
(( PAPER_FACTS )) && compgen -G "$LAST_PF/*.cast-paper-facts.json" >/dev/null && cp "$LAST_PF"/*.cast-paper-facts.json "$OUT/paper-facts/"

# Rebuild the index over the *published* summaries so its summaryFile paths
# stay valid after the work directory is removed.
if compgen -G "$OUT/summaries/*.cast-summary.json" >/dev/null; then
  run_index final "$INDEX" "$OUT/summaries" || true
fi
info "  import rounds completed: $DONE_ROUNDS"

finish 0

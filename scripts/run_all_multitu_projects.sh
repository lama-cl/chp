#!/usr/bin/env bash
set -u

# Run all tiny multi-TU test projects through the summary/export + iterative import/index workflow.
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

if ! [[ "$SUMMARY_IMPORT_ROUNDS" =~ ^[0-9]+$ ]]; then
  echo "error: SUMMARY_IMPORT_ROUNDS must be a non-negative integer: $SUMMARY_IMPORT_ROUNDS" >&2
  exit 1
fi

for bool_name in SUMMARY_REPORTS PAPER_FACTS RUNTIME_METRICS; do
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

collect_projects() {
  local roots=()
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
    find "$root" -mindepth 1 -maxdepth 1 -type d | sort
  done
}

collect_sources() {
  local project="$1"
  find "$project" -maxdepth 1 -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) | sort
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
if [[ "$RUNTIME_METRICS" == "1" ]]; then
  echo "  runtime aggregator:    $RUNTIME_AGGREGATOR"
fi
if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
  echo "  config id:            $SUMMARY_CONFIG_ID"
fi
echo

for project in "${projects[@]}"; do
  name="$(basename "$project")"
  safe="$(sanitize_name "$name")"
  project_out="$OUTPUT_ROOT/$safe"

  # Final/default outputs.
  final_text="$project_out"
  final_summaries="$project_out/summaries"
  final_reports="$project_out/reports"
  final_paper_facts="$project_out/paper-facts"
  final_runtime_metrics="$project_out/runtime-metrics"
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
    c2_txt="$project_out/c2-build-index.txt"
    rounds_root="$project_out"
  else
    tmp_root="$project_out/.tmp-intermediates"
    c1_text="$tmp_root/c1-text"
    c1_summaries="$tmp_root/c1-summaries"
    c1_reports="$tmp_root/c1-reports"
    c1_paper_facts="$tmp_root/c1-paper-facts"
    c1_runtime_metrics="$tmp_root/c1-runtime-metrics"
    c2_txt="$tmp_root/c2-build-index.txt"
    rounds_root="$tmp_root"
  fi

  rm -rf "$project_out"
  mkdir -p "$final_text" "$final_summaries" \
           "$c1_text" "$c1_summaries" "$rounds_root"
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

  if [[ ! -f "$project/compile_commands.json" ]]; then
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
    stem="$(sanitize_name "${src##*/}")"
    out_txt="$c1_text/${stem}.txt"
    cmd=("$TOOL" --summary-dir "$c1_summaries")
    if [[ "$SUMMARY_REPORTS" == "1" ]]; then
      cmd+=(--summary-report-dir "$c1_reports")
    fi
    if [[ "$PAPER_FACTS" == "1" ]]; then
      cmd+=(--paper-facts-dir "$c1_paper_facts")
    fi
    if [[ "$RUNTIME_METRICS" == "1" ]]; then
      cmd+=(--runtime-timing-dir "$c1_runtime_metrics"
            --runtime-timing-project-id "$safe"
            --runtime-timing-stage c1)
    fi
    if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
      cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-c1")
    fi
    cmd+=(-p "$project" "$src")
    echo "    C1 ${src##*/}"
    printf 'C1: ' >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
    if ! run_and_capture "$out_txt" "${cmd[@]}"; then
      echo "    C1 failed: $src"
      append_file_to_log "$log_file" "C1 failure output for ${src##*/}" "$out_txt"
      project_failed=1
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

    # C6g5t still performs exactly one final reporting overlay.  It consumes
    # C2's canonical local-only materialization (including fork/arm provenance)
    # but does not emit a new index or enter an import/rebuild loop.
    local_overlay_tag="local-index-overlay"
    local_overlay_text="$rounds_root/${local_overlay_tag}-text"
    local_overlay_summaries="$rounds_root/${local_overlay_tag}-summaries"
    local_overlay_reports="$rounds_root/${local_overlay_tag}-reports"
    local_overlay_paper_facts="$rounds_root/${local_overlay_tag}-paper-facts"
    local_overlay_runtime_metrics="$rounds_root/${local_overlay_tag}-runtime-metrics"
    mkdir -p "$local_overlay_text" "$local_overlay_summaries"
    if [[ "$SUMMARY_REPORTS" == "1" ]]; then
      mkdir -p "$local_overlay_reports"
    fi
    if [[ "$PAPER_FACTS" == "1" ]]; then
      mkdir -p "$local_overlay_paper_facts"
    fi
    if [[ "$RUNTIME_METRICS" == "1" ]]; then
      mkdir -p "$local_overlay_runtime_metrics"
    fi

    echo "    C2 local-only final-report overlay"
    for src in "${sources[@]}"; do
      stem="$(sanitize_name "${src##*/}")"
      out_txt="$local_overlay_text/${stem}.txt"
      cmd=("$TOOL" --enable-summary-import --summary-index "$index_for_next_round" --summary-dir "$local_overlay_summaries")
      if [[ "$SUMMARY_REPORTS" == "1" ]]; then
        cmd+=(--summary-report-dir "$local_overlay_reports")
      fi
      if [[ "$PAPER_FACTS" == "1" ]]; then
        cmd+=(--paper-facts-dir "$local_overlay_paper_facts")
      fi
      if [[ "$RUNTIME_METRICS" == "1" ]]; then
        cmd+=(--runtime-timing-dir "$local_overlay_runtime_metrics"
              --runtime-timing-project-id "$safe"
              --runtime-timing-stage "$local_overlay_tag")
      fi
      if [[ "$SUMMARY_IMPORT_DEBUG" == "1" ]]; then
        cmd+=(--summary-import-debug)
      fi
      if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
        cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-$local_overlay_tag")
      fi
      cmd+=(-p "$project" "$src")
      printf '%s: ' "$local_overlay_tag" >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
      if ! run_and_capture "$out_txt" "${cmd[@]}"; then
        echo "    $local_overlay_tag failed: $src"
        append_file_to_log "$log_file" "$local_overlay_tag failure output for ${src##*/}" "$out_txt"
        project_failed=1
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
        stem="$(sanitize_name "${src##*/}")"
        out_txt="$round_text/${stem}.txt"
        cmd=("$TOOL" --enable-summary-import --summary-index "$round_index_input" --summary-dir "$round_summaries")
        if [[ "$SUMMARY_REPORTS" == "1" ]]; then
          cmd+=(--summary-report-dir "$round_reports")
        fi
        if [[ "$PAPER_FACTS" == "1" ]]; then
          cmd+=(--paper-facts-dir "$round_paper_facts")
        fi
        if [[ "$RUNTIME_METRICS" == "1" ]]; then
          cmd+=(--runtime-timing-dir "$round_runtime_metrics"
                --runtime-timing-project-id "$safe"
                --runtime-timing-stage "$round_tag")
        fi
        if [[ "$SUMMARY_IMPORT_DEBUG" == "1" ]]; then
          cmd+=(--summary-import-debug)
        fi
        if [[ -n "$SUMMARY_CONFIG_ID" ]]; then
          cmd+=(--summary-config-id "$SUMMARY_CONFIG_ID-${round_tag}")
        fi
        cmd+=(-p "$project" "$src")
        echo "      ${round_tag} ${src##*/}"
        printf '%s: ' "$round_tag" >>"$log_file"; printf '%q ' "${cmd[@]}" >>"$log_file"; echo >>"$log_file"
        if ! run_and_capture "$out_txt" "${cmd[@]}"; then
          echo "    ${round_tag} failed: $src"
          append_file_to_log "$log_file" "${round_tag} failure output for ${src##*/}" "$out_txt"
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
    stem="$(sanitize_name "${src##*/}")"
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

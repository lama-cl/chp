#!/usr/bin/env bash
#
# build-compile-db-3c.sh
#
# Creates <project>/compile_commands.json for each top-level 3C benchmark
# project.  Prefer actual Bear-captured build commands; use a marked
# source-only fallback when no build system can be found or configured.
#
set -u
set -o pipefail

ROOT="${1:-.}"
ROOT="$(cd "$ROOT" && pwd -P)"
LOG_DIR="$ROOT/build-compile-db-logs"
JOBS="${JOBS:-$(nproc)}"

mkdir -p "$LOG_DIR"

success_build=()
success_fallback=()
failed=()
skipped=()

has_makefile() {
    local dir="$1"
    [[ -f "$dir/Makefile" ||
       -f "$dir/makefile" ||
       -f "$dir/GNUmakefile" ]]
}

repair_crlf_scripts() {
    local project="$1"
    local file

    while IFS= read -r -d '' file; do
        if LC_ALL=C grep -q $'\r' "$file"; then
            echo "  Converting CRLF to LF: ${file#$project/}"
            sed -i 's/\r$//' "$file"
        fi
    done < <(
        find "$project" -type f \
          \( -name configure \
             -o -name config.sub \
             -o -name config.guess \
             -o -name missing \
             -o -name install-sh \
             -o -name compile \
             -o -name depcomp \
             -o -name ltmain.sh \
             -o -name autogen.sh \
             -o -name bootstrap \
             -o -name '*.sh' \) \
          -print0
    )
}

run_logged() {
    local log_file="$1"
    shift

    {
        printf '+ '
        printf '%q ' "$@"
        printf '\n'
    } | tee -a "$log_file"

    "$@" 2>&1 | tee -a "$log_file"
    return "${PIPESTATUS[0]}"
}

find_build_root() {
    local project="$1"
    local candidate
    local -a candidates=()

    # Prefer the normal release layout.
    if [[ -f "$project/configure" ]] || has_makefile "$project"; then
        printf '%s\n' "$project"
        return 0
    fi

    # Lua and several older projects commonly build here.
    if [[ -d "$project/src" ]] &&
       { [[ -f "$project/src/configure" ]] || has_makefile "$project/src"; }; then
        printf '%s\n' "$project/src"
        return 0
    fi

    # Look for a single nested build root.
    mapfile -t candidates < <(
        find "$project" \
          -mindepth 1 \
          -maxdepth 3 \
          -type f \
          \( -name configure -o -name Makefile -o -name makefile -o -name GNUmakefile \) \
          -printf '%h\n' |
        sort -u
    )

    if [[ "${#candidates[@]}" -eq 1 ]]; then
        printf '%s\n' "${candidates[0]}"
        return 0
    fi

    if [[ "${#candidates[@]}" -gt 1 ]]; then
        echo "Multiple possible build roots:" >&2
        printf '  %s\n' "${candidates[@]}" >&2
    fi

    return 1
}

write_source_only_db() {
    local project="$1"
    local out_file="$2"

    python3 - "$project" "$out_file" <<'PY'
import json
import os
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()

skip_dirs = {
    ".git",
    ".svn",
    "build",
    "CMakeFiles",
    "build-compile-db-logs",
    "__pycache__",
}

sources = []
for current, dirs, files in os.walk(root):
    dirs[:] = [d for d in dirs if d not in skip_dirs]
    current_path = Path(current)

    for name in files:
        suffix = Path(name).suffix.lower()
        if suffix in {".c", ".cc", ".cpp", ".cxx"}:
            sources.append((current_path / name).resolve())

entries = []
for source in sorted(sources):
    compiler = "clang++" if source.suffix.lower() in {".cc", ".cpp", ".cxx"} else "clang"

    include_args = [
        f"-I{root}",
        f"-I{source.parent}",
    ]

    src_dir = root / "src"
    if src_dir.is_dir():
        include_args.append(f"-I{src_dir}")

    entries.append({
        "directory": str(source.parent),
        "arguments": [
            compiler,
            "-fsyntax-only",
            *include_args,
            str(source),
        ],
        "file": str(source),
    })

if not entries:
    raise SystemExit(f"No C/C++ source files found under {root}")

out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")

print(f"source-only entries: {len(entries)}")
PY
}

for project_path in "$ROOT"/*/; do
    [[ -d "$project_path" ]] || continue

    project="${project_path%/}"
    name="$(basename "$project")"
    log="$LOG_DIR/${name}-build-compile-db.log"
    out="$project/compile_commands.json"

    case "$name" in
        extra|build-compile-db-logs|behavior-tables|prevalence-tables)
            echo "Skipping $name"
            skipped+=("$name")
            continue
            ;;
    esac

    echo
    echo "============================================================"
    echo "Project: $name"
    echo "============================================================"

    {
        echo "Project: $name"
        echo "Root: $project"
        echo
    } > "$log"

    repair_crlf_scripts "$project"
    rm -f "$out"

    build_root=""
    if build_root="$(find_build_root "$project")"; then
        echo "Build root: ${build_root#$project/}" | tee -a "$log"

        (
            cd "$build_root" || exit 1

            # Old caches can preserve broken Windows/path assumptions.
            rm -f config.cache config.log config.status

            if [[ -f ./configure ]] && ! has_makefile "."; then
                chmod +x ./configure

                echo "Running configure..." | tee -a "$log"
                if ! run_logged "$log" env CC=clang CXX=clang++ ./configure; then
                    exit 10
                fi
            fi

            if ! has_makefile "."; then
                exit 11
            fi

            run_logged "$log" make clean || true

            echo "Capturing compilation commands with Bear..." | tee -a "$log"
            run_logged "$log" \
                bear --output "$out" -- \
                make -j"$JOBS" CC=clang CXX=clang++
        )

        status=$?

        if [[ "$status" -eq 0 && -s "$out" ]]; then
            echo "SUCCESS (Bear): $out"
            printf 'bear\n' > "$project/.compile-db-mode"
            success_build+=("$name")
            continue
        fi

        echo "Build capture was unavailable; using source-only fallback." | tee -a "$log"
    else
        echo "No unambiguous build root found; using source-only fallback." | tee -a "$log"
    fi

    if write_source_only_db "$project" "$out" 2>&1 | tee -a "$log"; then
        printf 'source-only-fallback\n' > "$project/.compile-db-mode"
        echo "SUCCESS (source-only fallback): $out"
        success_fallback+=("$name")
    else
        echo "FAILED: $name; see $log"
        failed+=("$name")
    fi
done

echo
echo "================ Summary ================"
echo "Bear build DBs (${#success_build[@]}): ${success_build[*]:-none}"
echo "Fallback DBs   (${#success_fallback[@]}): ${success_fallback[*]:-none}"
echo "Skipped        (${#skipped[@]}): ${skipped[*]:-none}"
echo "Failed         (${#failed[@]}): ${failed[*]:-none}"

[[ "${#failed[@]}" -eq 0 ]]
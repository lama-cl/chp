#!/usr/bin/env python3
"""
Patch run_all_multitu_projects-rp.sh for robust native sidecar staging.

This updater preserves the user's current RP runner and writes a new copy that:
  * injects --extra-arg-before=-resource-dir=<RESOURCE_DIR> into every
    compile-database TU parse;
  * creates analyzer JSON sidecars under native /tmp (or
    NATIVE_ARTIFACT_TMPDIR) instead of a mounted filesystem;
  * publishes the discovered sidecars into the existing output tree with
    collision-safe names; and
  * stages C1 prevalence JSON under the native directory before copying it to
    prevalence-metrics/.

It supports a current runner with or without the earlier resource-directory
patch. It is intentionally conservative: required C1 and import-round staging
anchors must be found before it writes output.
"""
from __future__ import annotations

import argparse
import re
import shutil
import stat
from pathlib import Path

PATCH_MARKER = "C6G5W1G_NATIVE_SIDECAR_STAGING"
RESOURCE_MARKER = "CLANG_RESOURCE_DIR_TOOL_ARGS"

RUNTIME_ASSIGN = 'RUNTIME_METRICS="${RUNTIME_METRICS:-0}"'
MAPFILE_PROJECTS = "mapfile -t projects < <(collect_projects)"

RESOURCE_BLOCK = r'''
# Clang builtin headers such as stddef.h live below the resource directory.
# Compile databases commonly omit -resource-dir because the compiler driver
# supplies it implicitly; LibTooling needs it explicitly for -p invocations.
CLANG_BIN="${CLANG_BIN:-clang}"
RESOURCE_DIR="${RESOURCE_DIR:-}"

if [[ -z "$RESOURCE_DIR" ]]; then
  if ! command -v "$CLANG_BIN" >/dev/null 2>&1; then
    echo "error: CLANG_BIN is not executable: $CLANG_BIN" >&2
    echo "       set CLANG_BIN or RESOURCE_DIR explicitly." >&2
    exit 1
  fi
  if ! RESOURCE_DIR="$("$CLANG_BIN" -print-resource-dir 2>/dev/null)"; then
    echo "error: could not determine the Clang resource directory" >&2
    echo "       set RESOURCE_DIR explicitly." >&2
    exit 1
  fi
fi

if [[ ! -f "$RESOURCE_DIR/include/stddef.h" ]]; then
  echo "error: invalid Clang resource directory: $RESOURCE_DIR" >&2
  echo "       expected: $RESOURCE_DIR/include/stddef.h" >&2
  exit 1
fi

CLANG_RESOURCE_DIR_TOOL_ARGS=(
  "--extra-arg-before=-resource-dir=$RESOURCE_DIR"
)
'''.lstrip("\n")

NATIVE_CONFIG_BLOCK = r'''
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
'''.lstrip("\n")

NATIVE_PUBLISH_BLOCK = r'''
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
'''.lstrip("\n")

PROJECT_NATIVE_STAGE_BLOCK = r'''
if ! native_artifact_tmp_root="$(
    mktemp -d "${NATIVE_ARTIFACT_TMPDIR%/}/cast-history-rp.XXXXXX"
  )"; then
  echo "error: could not create native artifact staging directory under: $NATIVE_ARTIFACT_TMPDIR" >&2
  exit 1
fi
NATIVE_ARTIFACT_TMP_ROOTS+=("$native_artifact_tmp_root")
printf 'Clang resource directory: %q\n' "$RESOURCE_DIR" >>"$log_file"
printf 'Native artifact staging root: %q\n' "$native_artifact_tmp_root" >>"$log_file"
'''.lstrip("\n")

PARSE_LINE = re.compile(
    r'^(?P<indent>[ \t]*)cmd\+=\(-p "\$(?P<project_var>[A-Za-z_][A-Za-z0-9_]*)" "\$src"\)[ \t]*$',
    re.MULTILINE,
)

STAGE_LINE = re.compile(
    r'^(?P<indent>[ \t]*)src_summary_dir="\$(?P<root>'
    r'c1_summary_tmp(?:_root)?|'
    r'local_overlay_summary_tmp_root|'
    r'round_summary_tmp_root'
    r')/\$\{stem\}"[ \t]*$',
    re.MULTILINE,
)

REPORT_STAGE_LINE = re.compile(
    r'^(?P<indent>[ \t]*)src_report_dir="\$(?P<root>'
    r'c1_report_tmp(?:_root)?|'
    r'local_overlay_report_tmp_root|'
    r'round_report_tmp_root'
    r')/\$\{stem\}"[ \t]*$',
    re.MULTILINE,
)

PREVALENCE_ASSIGN = re.compile(
    r'^(?P<indent>[ \t]*)explicit_prevalence_metrics="\$prevalence_metrics_dir/\$\{stem\}\.cast-prevalence\.json"[ \t]*$',
    re.MULTILINE,
)

PREVALENCE_CHECK = re.compile(
    r'^(?P<indent>[ \t]*)if \[\[ "\$PREVALENCE_METRICS" == "1" && ! -f "\$explicit_prevalence_metrics" \]\]; then\n'
    r'(?P<body>.*?)'
    r'^(?P=indent)fi[ \t]*$',
    re.MULTILINE | re.DOTALL,
)


def die(message: str) -> None:
    raise SystemExit(f"error: {message}")


def insert_after_exact_line(text: str, line: str, addition: str) -> str:
    count = text.count(line)
    if count != 1:
        die(f"expected exactly one configuration line {line!r}; found {count}")
    return text.replace(line, line + "\n\n" + addition.rstrip() + "\n", 1)


def add_resource_dir_support(text: str) -> tuple[str, int]:
    """Add the resource block when absent, then inject it at all parse sites."""
    if RESOURCE_MARKER not in text:
        text = insert_after_exact_line(text, RUNTIME_ASSIGN, RESOURCE_BLOCK)

    lines = text.splitlines(keepends=True)
    out: list[str] = []
    count = 0
    for line in lines:
        m = PARSE_LINE.match(line.rstrip("\n"))
        if m:
            previous = out[-1] if out else ""
            if RESOURCE_MARKER not in previous:
                out.append(f'{m.group("indent")}cmd+=("${{{RESOURCE_MARKER}[@]}}")\n')
            count += 1
        out.append(line)
    if count < 2:
        die(
            "found fewer than two compile-database parse sites. "
            "Expected C1 plus import replay; no output was written."
        )
    return "".join(out), count


def add_native_config(text: str) -> str:
    if PATCH_MARKER in text:
        die("runner already contains the native-sidecar-staging marker")
    if re.search(r'(?m)^\s*trap\b.*\bEXIT\b', text):
        die(
            "runner already defines an EXIT trap; this updater refuses to "
            "replace it. Please provide the runner for a manual merge."
        )
    return insert_after_exact_line(text, RUNTIME_ASSIGN, NATIVE_CONFIG_BLOCK)


def insert_publisher_overrides(text: str) -> str:
    count = text.count(MAPFILE_PROJECTS)
    if count != 1:
        die(f"expected exactly one project-loop anchor; found {count}")
    return text.replace(
        MAPFILE_PROJECTS,
        NATIVE_PUBLISH_BLOCK.rstrip() + "\n\n" + MAPFILE_PROJECTS,
        1,
    )


def insert_project_native_root(text: str) -> str:
    pattern = re.compile(r'^(?P<indent>[ \t]*): >"\$log_file"[ \t]*$', re.MULTILINE)
    matches = list(pattern.finditer(text))
    if len(matches) != 1:
        die(f"expected exactly one ': >\"$log_file\"' project-log anchor; found {len(matches)}")
    m = matches[0]
    indent = m.group("indent")
    block = "\n".join(
        (indent + line if line else line)
        for line in PROJECT_NATIVE_STAGE_BLOCK.rstrip().splitlines()
    )
    return text[:m.end()] + "\n" + block + text[m.end():]


def replace_stage_lines(text: str) -> tuple[str, int, int]:
    summary_counts = {"c1": 0, "local": 0, "round": 0}
    report_counts = {"c1": 0, "local": 0, "round": 0}

    def summary_repl(match: re.Match[str]) -> str:
        root = match.group("root")
        indent = match.group("indent")
        if root.startswith("c1_"):
            summary_counts["c1"] += 1
            stage = "c1"
        elif root.startswith("local_overlay"):
            summary_counts["local"] += 1
            stage = "${local_overlay_tag}"
        else:
            summary_counts["round"] += 1
            stage = "${round_tag}"
        return f'{indent}src_summary_dir="$native_artifact_tmp_root/{stage}/${{stem}}/sidecars"'

    def report_repl(match: re.Match[str]) -> str:
        root = match.group("root")
        indent = match.group("indent")
        if root.startswith("c1_"):
            report_counts["c1"] += 1
            stage = "c1"
        elif root.startswith("local_overlay"):
            report_counts["local"] += 1
            stage = "${local_overlay_tag}"
        else:
            report_counts["round"] += 1
            stage = "${round_tag}"
        return f'{indent}src_report_dir="$native_artifact_tmp_root/{stage}/${{stem}}/reports"'

    text, summary_total = STAGE_LINE.subn(summary_repl, text)
    text, report_total = REPORT_STAGE_LINE.subn(report_repl, text)

    if summary_counts["c1"] != 1:
        die(f"expected one C1 summary-stage assignment; found {summary_counts['c1']}")
    if summary_counts["round"] < 1:
        die("did not find an import-round summary-stage assignment")
    return text, summary_total, report_total


def stage_prevalence_metrics(text: str) -> tuple[str, bool]:
    """Stage direct C1 prevalence JSON natively when this runner supports it."""
    matches = list(PREVALENCE_ASSIGN.finditer(text))
    if not matches:
        return text, False
    if len(matches) != 1:
        die(f"expected one C1 prevalence assignment; found {len(matches)}")

    def assignment_repl(match: re.Match[str]) -> str:
        indent = match.group("indent")
        return (
            f'{indent}final_prevalence_metrics="$prevalence_metrics_dir/${{stem}}.cast-prevalence.json"\n'
            f'{indent}explicit_prevalence_metrics="$native_artifact_tmp_root/c1/${{stem}}/prevalence/${{stem}}.cast-prevalence.json"\n'
            f'{indent}mkdir -p "$(dirname "$explicit_prevalence_metrics")"'
        )

    text = PREVALENCE_ASSIGN.sub(assignment_repl, text, count=1)

    checks = list(PREVALENCE_CHECK.finditer(text))
    if len(checks) != 1:
        die(
            "found a prevalence output assignment but could not find its "
            "post-C1 missing-sidecar check for replacement"
        )

    check = checks[0]
    indent = check.group("indent")
    replacement = (
        f'{indent}if [[ "$PREVALENCE_METRICS" == "1" ]] && ! \\\n'
        f'{indent}    publish_native_prevalence_metrics \\\n'
        f'{indent}      "$explicit_prevalence_metrics" "$prevalence_metrics_dir" \\\n'
        f'{indent}      "$stem" "$log_file" "C1 ${{src##*/}}"; then\n'
        f'{indent}  echo "    C1 prevalence metrics sidecar missing: $src"\n'
        f'{indent}  {{\n'
        f'{indent}    echo\n'
        f'{indent}    echo "===== C1 prevalence metrics publish warning ====="\n'
        f'{indent}    echo "native staged sidecar: $explicit_prevalence_metrics"\n'
        f'{indent}    echo "final destination: $final_prevalence_metrics"\n'
        f'{indent}  }} >>"$log_file"\n'
        f'{indent}  project_failed=1\n'
        f'{indent}fi'
    )
    text = text[:check.start()] + replacement + text[check.end():]
    return text, True


def add_doc(text: str) -> str:
    anchor = re.compile(r'^(?P<indent>[ \t]*)#\s+RUNTIME_METRICS=0\b.*$', re.MULTILINE)
    matches = list(anchor.finditer(text))
    if len(matches) != 1:
        return text
    m = matches[0]
    indent = m.group("indent")
    addition = (
        f'\n{indent}#   RESOURCE_DIR=<clang-resource-dir>  # injected into all compile-database TU parses\n'
        f'{indent}#   NATIVE_ARTIFACT_TMPDIR=/tmp          # native staging for analyzer JSON sidecars'
    )
    return text[:m.end()] + addition + text[m.end():]


def verify(text: str) -> None:
    if RESOURCE_MARKER not in text or PATCH_MARKER not in text:
        die("internal verification failed: missing patch markers")
    parse_sites = list(PARSE_LINE.finditer(text))
    if len(parse_sites) < 2:
        die("internal verification failed: fewer than two compile-database parse sites remain")
    lines = text.splitlines()
    missing = []
    for idx, line in enumerate(lines):
        if PARSE_LINE.match(line):
            if idx == 0 or RESOURCE_MARKER not in lines[idx - 1]:
                missing.append(idx + 1)
    if missing:
        die(f"internal verification failed: resource-dir argument missing before parse lines {missing}")
    for needle in (
        '$native_artifact_tmp_root/c1/${stem}/sidecars',
        '$native_artifact_tmp_root/${round_tag}/${stem}/sidecars',
        'publish_rp_discovered_sidecar()',
        'Native artifact staging root:',
    ):
        if needle not in text:
            die(f"internal verification failed: expected native staging fragment absent: {needle}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a native-/tmp-staging and resource-directory-aware RP runner."
    )
    parser.add_argument(
        "runner",
        type=Path,
        help="path to the current run_all_multitu_projects-rp*.sh",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="output path; default: sibling <stem>-native-tmp.sh",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="replace the input runner after writing a .pre-native-tmp backup",
    )
    args = parser.parse_args()

    source = args.runner.resolve()
    if not source.is_file():
        die(f"runner does not exist: {source}")
    if args.in_place and args.output is not None:
        die("--in-place and --output cannot be used together")

    output = (
        source
        if args.in_place
        else (
            args.output.resolve()
            if args.output
            else source.with_name(source.stem + "-native-tmp" + source.suffix)
        )
    )

    text = source.read_text(encoding="utf-8")
    if PATCH_MARKER in text:
        die(f"{source} already contains {PATCH_MARKER}")

    text = add_doc(text)
    text, parse_sites = add_resource_dir_support(text)
    text = add_native_config(text)
    text = insert_publisher_overrides(text)
    text = insert_project_native_root(text)
    text, summary_sites, report_sites = replace_stage_lines(text)
    text, prevalence_staged = stage_prevalence_metrics(text)
    verify(text)

    if args.in_place:
        backup = source.with_name(source.name + ".pre-native-tmp")
        if backup.exists():
            die(f"backup already exists: {backup}")
        shutil.copy2(source, backup)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    output.chmod(stat.S_IMODE(source.stat().st_mode) | stat.S_IXUSR)

    print(f"wrote: {output}")
    print(f"resource-dir parse sites: {parse_sites}")
    print(f"native summary staging sites: {summary_sites}")
    print(f"native report staging sites: {report_sites}")
    print(f"native prevalence staging: {'yes' if prevalence_staged else 'not present in runner'}")
    if args.in_place:
        print(f"backup: {backup}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

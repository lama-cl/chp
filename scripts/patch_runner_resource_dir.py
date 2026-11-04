#!/usr/bin/env python3
"""
Create a resource-directory-aware copy of run_all_multitu_projects-rp.sh.

This updater is intentionally textual: it preserves the current runner,
including local C6g5t/C6g5w1e changes, and adds only the Clang resource-dir
plumbing needed by compile-database analyses.
"""
from __future__ import annotations

import argparse
import re
import shutil
import stat
from pathlib import Path

RUNTIME_METRICS_LINE = 'RUNTIME_METRICS="${RUNTIME_METRICS:-0}"'
PATCH_MARKER = "CLANG_RESOURCE_DIR_TOOL_ARGS"

RESOURCE_BLOCK = r'''
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
'''.lstrip("\n")

COMMAND_PATTERN = re.compile(
    r'^(?P<indent>[ \t]*)cmd\+=\(-p "\$(?P<dir>[A-Za-z_][A-Za-z0-9_]*)" "\$src"\)[ \t]*$',
    re.MULTILINE,
)

DOC_ANCHOR_PATTERN = re.compile(
    r'^(?P<indent>[ \t]*)#\s+RUNTIME_METRICS=0\b.*$',
    re.MULTILINE,
)

LOG_ANCHOR_PATTERN = re.compile(
    r'^(?P<indent>[ \t]*): >"\$log_file"[ \t]*$',
    re.MULTILINE,
)


def die(message: str) -> None:
    raise SystemExit(f"error: {message}")


def insert_after_exact_line(text: str, line: str, addition: str) -> str:
    occurrences = text.count(line)
    if occurrences != 1:
        die(f"expected exactly one {line!r} assignment, found {occurrences}")
    return text.replace(line, line + "\n\n" + addition.rstrip() + "\n", 1)


def add_doc(text: str) -> str:
    matches = list(DOC_ANCHOR_PATTERN.finditer(text))
    if len(matches) != 1:
        # Documentation is not essential to correctness. Preserve the runner
        # even if a local branch changed the option-comment wording.
        return text
    match = matches[0]
    indent = match.group("indent")
    addition = (
        f"\n{indent}#   CLANG_BIN=clang                    # executable used when RESOURCE_DIR is not set"
        f"\n{indent}#   RESOURCE_DIR=<clang-resource-dir>  # injected for all compile-database analyzer invocations"
    )
    return text[:match.end()] + addition + text[match.end():]


def add_log_metadata(text: str) -> str:
    matches = list(LOG_ANCHOR_PATTERN.finditer(text))
    if len(matches) != 1:
        return text
    match = matches[0]
    indent = match.group("indent")
    replacement = (
        match.group(0)
        + f'\n{indent}printf \'Clang resource directory: %q\\n\' "$RESOURCE_DIR" >>"$log_file"'
    )
    return text[:match.start()] + replacement + text[match.end():]


def add_tool_args_before_parse_sites(text: str) -> tuple[str, int]:
    if PATCH_MARKER not in text:
        die("resource-dir block was not inserted before command patching")

    def replace(match: re.Match[str]) -> str:
        indent = match.group("indent")
        return (
            f'{indent}cmd+=("${{{PATCH_MARKER}[@]}}")\n'
            + match.group(0)
        )

    patched, count = COMMAND_PATTERN.subn(replace, text)
    if count < 2:
        die(
            "found fewer than two analyzer compile-database parse sites; "
            "expected C1 plus import replay. No output was written."
        )
    return patched, count


def verify(text: str, expected_sites: int) -> None:
    if PATCH_MARKER not in text:
        die("internal verification failed: resource-dir argument array is absent")

    parse_sites = list(COMMAND_PATTERN.finditer(text))
    if len(parse_sites) != expected_sites:
        die(
            "internal verification failed: command-site count changed "
            f"from {expected_sites} to {len(parse_sites)}"
        )

    missing: list[int] = []
    lines = text.splitlines()
    for line_number, line in enumerate(lines, start=1):
        if re.match(r'^[ \t]*cmd\+=\(-p "\$[A-Za-z_][A-Za-z0-9_]*" "\$src"\)[ \t]*$', line):
            previous = lines[line_number - 2] if line_number >= 2 else ""
            if PATCH_MARKER not in previous:
                missing.append(line_number)
    if missing:
        die(
            "internal verification failed: resource-dir arguments were not "
            f"inserted immediately before parse sites on lines {missing}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a resource-directory-aware copy of run_all_multitu_projects-rp.sh."
    )
    parser.add_argument("runner", type=Path, help="path to the current run_all_multitu_projects-rp.sh")
    parser.add_argument(
        "-o", "--output", type=Path,
        help="output path; default: sibling run_all_multitu_projects-rp-resource-dir.sh",
    )
    parser.add_argument(
        "--in-place", action="store_true",
        help="replace the input runner after writing a .pre-resource-dir backup",
    )
    args = parser.parse_args()

    source = args.runner.resolve()
    if not source.is_file():
        die(f"runner does not exist: {source}")
    if args.in_place and args.output is not None:
        die("--in-place and --output cannot be used together")

    default_output = source.with_name(source.stem + "-resource-dir" + source.suffix)
    output = source if args.in_place else (args.output.resolve() if args.output else default_output)

    text = source.read_text(encoding="utf-8")
    if PATCH_MARKER in text:
        die(f"{source} already contains {PATCH_MARKER}; refusing to patch twice")

    text = add_doc(text)
    text = insert_after_exact_line(text, RUNTIME_METRICS_LINE, RESOURCE_BLOCK)
    text = add_log_metadata(text)
    text, site_count = add_tool_args_before_parse_sites(text)
    verify(text, site_count)

    if args.in_place:
        backup = source.with_name(source.name + ".pre-resource-dir")
        if backup.exists():
            die(f"backup already exists: {backup}")
        shutil.copy2(source, backup)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    output.chmod(stat.S_IMODE(source.stat().st_mode) | stat.S_IXUSR)

    print(f"wrote: {output}")
    print(f"patched compile-database analyzer command sites: {site_count}")
    print("injected argument: --extra-arg-before=-resource-dir=<RESOURCE_DIR>")
    if args.in_place:
        print(f"backup: {backup}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

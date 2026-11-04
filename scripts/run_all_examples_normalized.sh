#!/usr/bin/env bash
set -u

# Run from the build directory, for example:
#   cd build
#   ../scripts/run_all_examples_normalized.sh
#
# Expected layout:
#   build/cast-history-tool
#   examples/*.c
#   examples-output/
#
# This version normalizes build/project-specific absolute paths in the output
# so diffs are not polluted by directory names such as:
#   v10-bounded-transitive-alias-chain
#   v11-recursive-functions

TOOL="${TOOL:-./cast-history-tool}"
EXAMPLES_DIR="${EXAMPLES_DIR:-../examples}"
OUTPUT_DIR="${OUTPUT_DIR:-../examples-output}"
STD="${STD:--std=c17}"

if [[ ! -x "$TOOL" ]]; then
  echo "error: tool not found or not executable: $TOOL" >&2
  echo "hint: run this script from the build directory or set TOOL=/path/to/cast-history-tool" >&2
  exit 1
fi

if [[ ! -d "$EXAMPLES_DIR" ]]; then
  echo "error: examples directory not found: $EXAMPLES_DIR" >&2
  echo "hint: set EXAMPLES_DIR=/path/to/examples" >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

# Canonical paths used for normalization.
BUILD_ABS="$(pwd -P)"
EXAMPLES_ABS="$(cd "$EXAMPLES_DIR" && pwd -P)"
PROJECT_ABS="$(cd "$EXAMPLES_DIR/.." && pwd -P)"

# Escape a string for sed search pattern.
escape_sed() {
  printf '%s' "$1" | sed -e 's/[\/&]/\\&/g'
}

BUILD_ABS_ESC="$(escape_sed "$BUILD_ABS")"
EXAMPLES_ABS_ESC="$(escape_sed "$EXAMPLES_ABS")"
PROJECT_ABS_ESC="$(escape_sed "$PROJECT_ABS")"

normalize_output() {
  # Normalize the most specific paths first.
  #
  # Typical Clang source locations may look like:
  #   /mnt/.../v11-recursive-functions/build/../examples/foo.c:1:2
  # or:
  #   /mnt/.../v11-recursive-functions/examples/foo.c:1:2
  #
  # Convert both to:
  #   ../examples/foo.c:1:2
  sed \
    -e "s/${BUILD_ABS_ESC}\/\.\.\/examples/..\/examples/g" \
    -e "s/${EXAMPLES_ABS_ESC}/..\/examples/g" \
    -e "s/${PROJECT_ABS_ESC}/../g"
}

shopt -s nullglob
files=("$EXAMPLES_DIR"/*.c)

if [[ ${#files[@]} -eq 0 ]]; then
  echo "warning: no .c files found in $EXAMPLES_DIR" >&2
  exit 0
fi

passed=0
failed=0

echo "Running ${#files[@]} example(s)"
echo "  tool:        $TOOL"
echo "  examples:    $EXAMPLES_DIR"
echo "  output dir:  $OUTPUT_DIR"
echo "  std:         $STD"
echo "  normalized:  yes"
echo

for src in "${files[@]}"; do
  base="$(basename "$src" .c)"
  out="$OUTPUT_DIR/${base}-output.txt"
  tmp="${out}.tmp"

  echo "==> $src"
  echo "    -> $out"

  {
    echo "Command:"
    echo "$TOOL --debug-dump \"$src\" -- $STD"
    echo
    echo "Output:"
    "$TOOL" --debug-dump "$src" -- "$STD"
  } >"$tmp" 2>&1

  status=$?
  normalize_output <"$tmp" >"$out"
  rm -f "$tmp"

  if [[ $status -eq 0 ]]; then
    ((passed++))
  else
    ((failed++))
    echo "    FAILED with exit code $status"
    echo "    see $out"
  fi
done

echo
echo "Done."
echo "  passed: $passed"
echo "  failed: $failed"
echo "  outputs written to: $OUTPUT_DIR"

if [[ $failed -ne 0 ]]; then
  exit 1
fi

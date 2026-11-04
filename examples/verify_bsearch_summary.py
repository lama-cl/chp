#!/usr/bin/env python3
"""Verify the C6g4d bsearch pointer-arithmetic projection regression.

Usage:
  python3 examples/verify_bsearch_summary.py path/to/bsearch-output.txt
"""

from __future__ import annotations

import sys
from pathlib import Path


def normalize(text: str) -> str:
    return " ".join(text.split())


def require(text: str, fragment: str, description: str) -> None:
    if normalize(fragment) not in normalize(text):
        raise AssertionError(f"missing {description}:\n  {fragment}")


def forbid(text: str, fragment: str, description: str) -> None:
    if normalize(fragment) in normalize(text):
        raise AssertionError(f"unexpected {description}:\n  {fragment}")


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    output_path = Path(sys.argv[1])
    text = output_path.read_text(encoding="utf-8")

    expected_base = (
        "bsearch::__base : const void * => const char * => "
        "ptrarith(const char *) => const void * => $4.$1"
    )
    expected_param = (
        "$1 : const void * => const char * => ptrarith(const char *) "
        "=> const void * => $4.$1"
    )

    require(text, expected_base, "fully extended __base history")
    require(text, expected_param, "fully extended bsearch parameter $1 summary")
    forbid(
        text,
        "bsearch::__base : const void * => ⟨| const char * => const void *,",
        "spurious parallel __base projection arm",
    )
    forbid(
        text,
        "$1 : const void * => ⟨| const char * => const void *,",
        "spurious parallel parameter-$1 projection arm",
    )

    print("bsearch C6g4d regression checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, AssertionError) as exc:
        print(f"bsearch C6g4d regression check failed: {exc}", file=sys.stderr)
        raise SystemExit(1)

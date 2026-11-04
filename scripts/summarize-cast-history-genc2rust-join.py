#!/usr/bin/env python3
"""Summarize the classification<->taints join, grouped by family.

Input is the CSV produced by join-post-summary.py. Output is a CSV with one row
per cast-history semantic family:

    Cast-history semantic family,
    GenC2rust-generic,
    GenC2rust Non-generic,
    GenC2rust Unhandled or Unsupported,
    GenC2rust Mixed / cannot determine,
    Total (generic + non-generic + unhandled/unsupported + mixed)

Two counting modes
------------------
Default (per-location): the unit is a definition location (family + line/col).
For each location, the distinct outcome buckets across its taint entries are
collected. If they all agree on one bucket, the location counts once in that
bucket; if they disagree, the location counts once in "Mixed / cannot
determine". Example: line 1842 col 41 has both generic and unsupported entries
(buckets generic + unhandled), so it is reported as mixed=1.

--per-taint (fan-out): the unit is a single taint entry. Each in-consideration
joined row adds one to its bucket, so a location with several classifications
adds to several buckets and the Mixed column is always 0. Example: line 1842
col 41 is reported as 4 generic and 3 unsupported.

Buckets / "mixed"
-----------------
A taint's `classification` is bucketed as:
    generic        -> generic, type_param/typeparam
    non-generic    -> non_generic, concrete, actual(_type)
    everything else in-consideration (unsupported, unretyped, void, conflict,
                   blank, unknown) -> Unhandled or Unsupported

"Mixed" is decided on these buckets, not the raw strings: a location is mixed
only when its entries disagree on the bucket (e.g. generic + unhandled, or
generic + non-generic). A location whose entries are only unsupported/unretyped
is unhandled, not mixed -- there is no conflict about the outcome.

Rows marked "not in genc2rust consideration" (definitions GenC2Rust never saw)
are skipped in both modes; they are detected on the `classification` column.

Examples:
  python3 summarize-join-by-family.py joined.csv > summary.csv
  python3 summarize-join-by-family.py joined.csv --per-taint > summary.csv
  python3 join-post-summary.py classes.csv taints.csv | \
      python3 summarize-join-by-family.py - --out summary.csv
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

MARKER_KEY = "not_in_genc2rust_consideration"

GENERIC = {"generic", "type_param", "typeparam", "tparam"}
NON_GENERIC = {"non_generic", "nongeneric", "concrete", "actual", "actual_type",
               "actualtype"}

# index into the per-family count vector [generic, nongeneric, unhandled, mixed]
BUCKET_IDX = {"generic": 0, "nongeneric": 1, "unhandled": 2}
MIXED_IDX = 3

COL_FAMILY = "Cast-history semantic family"
COL_GENERIC = "GenC2rust-generic"
COL_NONGENERIC = "GenC2rust Non-generic"
COL_UNHANDLED = "GenC2rust Unhandled or Unsupported"
COL_MIXED = "GenC2rust Mixed / cannot determine"
COL_TOTAL = "Total (generic + non-generic + unhandled/unsupported + mixed)"
OUT_FIELDS = [COL_FAMILY, COL_GENERIC, COL_NONGENERIC, COL_UNHANDLED, COL_MIXED,
              COL_TOTAL]


def norm(value: str | None) -> str:
    """lowercase, trim, and unify separators for tolerant matching."""
    if value is None:
        return ""
    return value.strip().lower().replace("-", "_").replace(" ", "_")


def bucket(classification: str | None) -> str | None:
    """Map a classification value to 'generic'/'nongeneric'/'unhandled'.

    Returns None for the not-in-consideration marker (caller skips it).
    """
    key = norm(classification)
    if key == MARKER_KEY:
        return None
    if key in GENERIC:
        return "generic"
    if key in NON_GENERIC:
        return "nongeneric"
    return "unhandled"


def open_input(path: Path):
    if str(path) == "-":
        return sys.stdin
    return path.open("r", newline="", encoding="utf-8")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("joined", type=Path,
                    help="CSV from join-post-summary.py (use - for stdin)")
    ap.add_argument("--out", type=Path, help="write CSV here instead of stdout")
    ap.add_argument("--per-taint", action="store_true",
                    help="count each taint classification entry separately "
                         "(fan-out); a location with multiple classifications "
                         "adds to each bucket instead of the Mixed/cannot-"
                         "determine column (which is then always 0)")
    ap.add_argument("--family-col", default="family",
                    help="family column in the joined CSV (default: family)")
    ap.add_argument("--classification-col", default="classification",
                    help="classification column (default: classification)")
    ap.add_argument("--line-col", default="defStartLine",
                    help="location line column, used to group per-location in "
                         "default mode (default: defStartLine)")
    ap.add_argument("--col-col", default="defStartColumn",
                    help="location column column, used to group per-location in "
                         "default mode (default: defStartColumn)")
    ap.add_argument("--no-grand-total", action="store_true",
                    help="omit the trailing all-families total row")
    args = ap.parse_args(argv)

    fh = open_input(args.joined)
    try:
        reader = csv.DictReader(fh)
        fields = reader.fieldnames or []
        if args.family_col not in fields:
            raise SystemExit(
                f"error: joined CSV has no {args.family_col!r} column; "
                f"available: {', '.join(fields)}"
            )
        if args.classification_col not in fields:
            raise SystemExit(
                f"error: joined CSV has no {args.classification_col!r} column. "
                "This summary needs GenC2Rust's classification; regenerate the "
                "join from a taints CSV that includes the classification "
                "column."
            )
        if not args.per_taint:
            for c in (args.line_col, args.col_col):
                if c not in fields:
                    raise SystemExit(
                        f"error: joined CSV has no {c!r} column, needed to group "
                        "per location in the default (mixed) mode; pass "
                        "--line-col/--col-col or use --per-taint. "
                        f"available: {', '.join(fields)}"
                    )

        # family -> [generic, nongeneric, unhandled, mixed]
        counts: dict[str, list[int]] = {}
        order: list[str] = []

        def ensure(fam: str) -> list[int]:
            if fam not in counts:
                counts[fam] = [0, 0, 0, 0]
                order.append(fam)
            return counts[fam]

        n_rows = n_skipped = 0

        if args.per_taint:
            for row in reader:
                n_rows += 1
                b = bucket(row.get(args.classification_col))
                if b is None:
                    n_skipped += 1
                    continue
                fam = (row.get(args.family_col) or "").strip()
                ensure(fam)[BUCKET_IDX[b]] += 1
            n_unit = n_rows - n_skipped
            n_mixed = 0
        else:
            # (family, line, col) -> set of buckets seen at that location
            groups: dict[tuple[str, str, str], set[str]] = {}
            for row in reader:
                n_rows += 1
                b = bucket(row.get(args.classification_col))
                if b is None:
                    n_skipped += 1
                    continue
                fam = (row.get(args.family_col) or "").strip()
                key = (fam,
                       (row.get(args.line_col) or "").strip(),
                       (row.get(args.col_col) or "").strip())
                groups.setdefault(key, set()).add(b)
            n_mixed = 0
            for (fam, _ln, _cl), buckets in groups.items():
                vec = ensure(fam)
                if len(buckets) == 1:
                    vec[BUCKET_IDX[next(iter(buckets))]] += 1
                else:
                    vec[MIXED_IDX] += 1
                    n_mixed += 1
            n_unit = len(groups)
    finally:
        if fh is not sys.stdin:
            fh.close()

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        out_fh = args.out.open("w", newline="", encoding="utf-8")
        close = True
    else:
        out_fh, close = sys.stdout, False

    try:
        writer = csv.DictWriter(out_fh, fieldnames=OUT_FIELDS)
        writer.writeheader()
        grand = [0, 0, 0, 0]
        for fam in sorted(order, key=lambda f: f.lower()):
            g, ng, uh, mx = counts[fam]
            for i, v in enumerate((g, ng, uh, mx)):
                grand[i] += v
            writer.writerow({
                COL_FAMILY: fam,
                COL_GENERIC: g,
                COL_NONGENERIC: ng,
                COL_UNHANDLED: uh,
                COL_MIXED: mx,
                COL_TOTAL: g + ng + uh + mx,
            })
        if not args.no_grand_total:
            writer.writerow({
                COL_FAMILY: "Total (all families)",
                COL_GENERIC: grand[0],
                COL_NONGENERIC: grand[1],
                COL_UNHANDLED: grand[2],
                COL_MIXED: grand[3],
                COL_TOTAL: sum(grand),
            })
    finally:
        if close:
            out_fh.close()

    mode = "per-taint" if args.per_taint else "per-location"
    unit = "taint entr" + ("y" if n_unit == 1 else "ies") if args.per_taint \
        else "location(s)"
    extra = "" if args.per_taint else f", {n_mixed} mixed"
    print(
        f"summarized {n_rows} joined row(s) [{mode}]: {n_unit} in-consideration "
        f"{unit} across {len(order)} family(ies){extra}, {n_skipped} skipped "
        f"(not in genc2rust consideration)",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

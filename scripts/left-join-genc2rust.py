#!/usr/bin/env python3
"""Left-join stable-classification CSV with GenC2Rust's post_summary_log.csv.

Both inputs are CSV; the output is CSV.

The left table is produced by extract-stableclassification.py. The right table
is GenC2Rust's post_summary_log.csv, whose columns are (in the version this was
written against):

    begin_line, begin_col, end_line, end_column, label_id, node, group,
    function_name, causes, classification, inferred_type

The join is a LEFT join keyed on (line, column):

    left  defStartLine / defStartColumn   ==   right  begin_line / begin_col

Matching is one-row-per-match (fan-out): if one (line, column) pair has several
post_summary_log rows, the left row is repeated once per matching right row.
Left rows with no match in post_summary_log are kept, with every genc2rust
column set to the literal "not in genc2rust consideration".

Notes / caveats:
  * post_summary_log has no source-file column, so the join is purely on
    (line, column). If your run spans multiple source files, identical
    (line, column) pairs in different files can collide. There is nothing to
    disambiguate them with here.
  * GenC2Rust writes every field with Rust's `{:?}`, so string fields arrive
    quote-wrapped; Python's csv reader unwraps that for the common case. A
    Debug-formatted label_id/node/group value that contains a comma would still
    break column alignment upstream of this script -- it cannot be recovered
    here.

Examples:
  python3 join-post-summary.py classifications.csv post_summary_log.csv > joined.csv
  python3 join-post-summary.py classifications.csv post_summary_log.csv --out joined.csv
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path
from typing import Any

UNMATCHED = "not in genc2rust consideration"


def normalize_header(name: str) -> str:
    """Canonicalize a header cell: lowercase, trim, spaces/hyphens -> underscore."""
    out = name.strip().lower().replace("-", "_").replace(" ", "_")
    while "__" in out:
        out = out.replace("__", "_")
    return out


def key_part(value: Any) -> Any:
    """Normalize one half of the (line, column) key.

    Compare as integers when possible so "13", " 13 " and 13 all agree;
    fall back to the stripped string otherwise. Empty -> None (never matches).
    """
    if value is None:
        return None
    s = str(value).strip()
    if s == "":
        return None
    try:
        return int(s)
    except ValueError:
        return s


def read_post_summary(
    path: Path, line_col: str, col_col: str
) -> tuple[list[str], dict[tuple[Any, Any], list[dict[str, str]]]]:
    """Read post_summary_log into (ordered genc2rust columns, key -> rows)."""
    with path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.reader(fh)
        try:
            raw_header = next(reader)
        except StopIteration:
            raise SystemExit(f"error: {path} is empty")
        header = [normalize_header(h) for h in raw_header]
        if line_col not in header:
            raise SystemExit(
                f"error: post_summary_log column {line_col!r} not found; "
                f"available: {', '.join(header)}"
            )
        if col_col not in header:
            raise SystemExit(
                f"error: post_summary_log column {col_col!r} not found; "
                f"available: {', '.join(header)}"
            )
        li = header.index(line_col)
        ci = header.index(col_col)

        index: dict[tuple[Any, Any], list[dict[str, str]]] = {}
        for raw in reader:
            if not raw:
                continue
            # Pad/truncate defensively so a malformed row never crashes the join.
            cells = (raw + [""] * len(header))[: len(header)]
            row = {header[i]: cells[i] for i in range(len(header))}
            k = (key_part(cells[li]), key_part(cells[ci]))
            if k[0] is None or k[1] is None:
                continue
            index.setdefault(k, []).append(row)
    return header, index


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("classifications", type=Path,
                    help="CSV from extract-stableclassification.py (left table)")
    ap.add_argument("post_summary_log", type=Path,
                    help="GenC2Rust post_summary_log.csv (right table)")
    ap.add_argument("--out", type=Path, help="write CSV here instead of stdout")
    ap.add_argument("--left-line", default="defStartLine",
                    help="left line-key column (default: defStartLine)")
    ap.add_argument("--left-col", default="defStartColumn",
                    help="left column-key column (default: defStartColumn)")
    ap.add_argument("--right-line", default="begin_line",
                    help="right line-key column (default: begin_line)")
    ap.add_argument("--right-col", default="begin_col",
                    help="right column-key column (default: begin_col)")
    args = ap.parse_args(argv)

    right_line = normalize_header(args.right_line)
    right_col = normalize_header(args.right_col)
    g2r_columns, index = read_post_summary(
        args.post_summary_log, right_line, right_col
    )

    with args.classifications.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        left_fields = list(reader.fieldnames or [])
        if args.left_line not in left_fields or args.left_col not in left_fields:
            raise SystemExit(
                f"error: classifications CSV missing key columns "
                f"{args.left_line!r}/{args.left_col!r}; "
                f"available: {', '.join(left_fields)}"
            )

        # Output schema: all left columns, then the genc2rust columns. On the
        # (unlikely) chance a genc2rust column name collides with a left one,
        # prefix it with g2r_ so nothing is silently overwritten.
        left_set = set(left_fields)
        out_g2r: list[str] = []
        rename: dict[str, str] = {}
        for c in g2r_columns:
            name = f"g2r_{c}" if c in left_set else c
            rename[c] = name
            out_g2r.append(name)
        out_fields = left_fields + out_g2r

        if args.out:
            args.out.parent.mkdir(parents=True, exist_ok=True)
            out_fh = args.out.open("w", newline="", encoding="utf-8")
            close = True
        else:
            out_fh = sys.stdout
            close = False

        n_left = n_out = n_matched_left = n_unmatched_left = 0
        try:
            writer = csv.DictWriter(out_fh, fieldnames=out_fields)
            writer.writeheader()
            for row in reader:
                n_left += 1
                base = {k: row.get(k, "") for k in left_fields}
                k = (key_part(row.get(args.left_line)),
                     key_part(row.get(args.left_col)))
                matches = index.get(k) if k[0] is not None and k[1] is not None else None
                if matches:
                    n_matched_left += 1
                    for m in matches:
                        out = dict(base)
                        for c in g2r_columns:
                            out[rename[c]] = m.get(c, "")
                        writer.writerow(out)
                        n_out += 1
                else:
                    n_unmatched_left += 1
                    out = dict(base)
                    for c in g2r_columns:
                        out[rename[c]] = UNMATCHED
                    writer.writerow(out)
                    n_out += 1
        finally:
            if close:
                out_fh.close()

    print(
        f"joined {n_left} classification row(s) -> {n_out} output row(s) "
        f"({n_matched_left} matched, {n_unmatched_left} unmatched) "
        f"against {sum(len(v) for v in index.values())} post_summary_log row(s)",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

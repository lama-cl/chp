#!/usr/bin/env python3
"""Left-join stable- and void-classification CSVs against the 3C/GenC2Rust cross
table (output of ``join-3c-genc2rust.py``, assumed to carry ``project``, ``file``
and ``Name``), producing two 3-way outputs.

Each classification row is preserved (left join) and enriched with:

    match rule                 <- cross table match_rule (blank if no match)
    genc2rust-match status      matched-genc2rust | nomatch-genc2rust
    genc2rust-classification    <- cross classification, or "not matched with genc2rust"
    3c-match status             match-3c | nomatch-3c
    3c-classification           wild | not wild
    3c-reasons                  <- cross Reasons        (blank if no 3c)
    3c-ptrs affected            <- cross PtrsAffected    (blank if no 3c)
    3c-ptrs score               <- cross PtrsScore       (blank if no 3c)
    3c-name                     <- cross 3C Name         (blank if no 3c)
    name-match                  match | mismatch | ""    (audit only, never filters)

"genc2rust present" means the cross row's ``match`` is ``both``/``genc2rust-only``;
"3c present" means ``both``/``3c-only``.

Location matching (per classification row)
------------------------------------------
Match on file + begin ``(line, col)`` first; else fall back to file + ``line``.
Files match by trailing path suffix: the shorter path must be a component-suffix
of the longer, so ``long/dir/subdir/file.c`` matches ``subdir/file.c`` and
``file.c``, but ``x/file.c`` does NOT match ``y/file.c``. ``project`` is not in the
key. Fan-out: one output row per matched cross row; none -> one all-nomatch row.

name-match (audit)
------------------
Compares the identity of the 3C ``Name`` with the classification subject. Identity
is the token after the last ``:`` (``AVTActionFlush::tmp`` -> ``tmp``,
``10:14:return`` -> ``return``, plain ``x`` -> ``x``); the 3C name also has a
leading ``>>``/``<<`` and a trailing ``_<id>`` stripped, and any ``$ret`` is the
return. Records ``match``/``mismatch`` only; never removes a row.

  * stable_classification keys on defFile / defStartLine / defStartColumn; identity from ``subject``
  * void_classification   keys on source_file / source_line / source_column; identity from ``declaration_name``

Outputs (into --out): join-sc-3way.csv, join-vc-3way.csv.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

APPENDED = [
    "match rule",
    "genc2rust-match status",
    "genc2rust-classification",
    "3c-match status",
    "3c-classification",
    "3c-reasons",
    "3c-ptrs affected",
    "3c-ptrs score",
    "3c-name",
    "name-match",
]

GENC_MATCH = "matched-genc2rust"
GENC_NOMATCH = "nomatch-genc2rust"
GENC_MISSING = "not matched with genc2rust"
THREEC_MATCH = "match-3c"
THREEC_NOMATCH = "nomatch-3c"
THREEC_WILD = "wild"
THREEC_NOTWILD = "not wild"

NOMATCH_APPENDED = {
    "match rule": "",
    "genc2rust-match status": GENC_NOMATCH,
    "genc2rust-classification": GENC_MISSING,
    "3c-match status": THREEC_NOMATCH,
    "3c-classification": THREEC_NOTWILD,
    "3c-reasons": "",
    "3c-ptrs affected": "",
    "3c-ptrs score": "",
    "3c-name": "",
    "name-match": "",
}

_ID_SUFFIX = re.compile(r"_\d+$")


def norm_header(name: str) -> str:
    return name.strip().lower().replace(" ", "_").replace("-", "_")


def resolve_col(fieldnames: list[str], logical: str, *, where: str) -> str:
    want = norm_header(logical)
    lookup = {norm_header(f): f for f in fieldnames}
    if want not in lookup:
        raise SystemExit(
            f"error: {where} has no column matching '{logical}'. "
            f"Available: {', '.join(fieldnames)}"
        )
    return lookup[want]


def to_int(v: str | None) -> int | None:
    if v is None:
        return None
    v = v.strip()
    try:
        return int(v)
    except ValueError:
        return None


def path_components(p: str) -> list[str]:
    return [c for c in re.split(r"[/\\]", (p or "").strip()) if c not in ("", ".")]


def basename_of(p: str) -> str:
    comps = path_components(p)
    return comps[-1] if comps else ""


def file_suffix_match(a: str, b: str) -> bool:
    """True if the shorter path is a trailing-component suffix of the longer."""
    ca, cb = path_components(a), path_components(b)
    if not ca or not cb:
        return False
    n = min(len(ca), len(cb))
    return ca[-n:] == cb[-n:]


def subject_identity(value: str) -> tuple[str | None, bool]:
    v = (value or "").strip()
    if not v:
        return None, False
    leaf = v.rsplit(":", 1)[-1].strip()
    return (leaf or None), (leaf.lower() == "return")


def name_identity_3c(name: str) -> tuple[str | None, bool]:
    n = (name or "").strip()
    if not n:
        return None, False
    if "$ret" in n:
        return "return", True
    if n.startswith(">>") or n.startswith("<<"):
        n = n[2:]
    part = n.rsplit(":", 1)[-1] if ":" in n else n
    part = _ID_SUFFIX.sub("", part)
    if not part and ":" in n:
        part = _ID_SUFFIX.sub("", n.rsplit(":", 1)[0])
    return (part or None), False


def name_match(class_id, three_id) -> str:
    cl_leaf, cl_ret = class_id
    tc_leaf, tc_ret = three_id
    if tc_leaf is None or cl_leaf is None:
        return ""
    if cl_ret or tc_ret:
        return "match" if (cl_ret and tc_ret) else "mismatch"
    return "match" if cl_leaf == tc_leaf else "mismatch"


def build_cross_index(path: Path, cols: dict[str, str]):
    """Return (by_exact, by_line, appended, three_ids, files).

    Buckets by (basename, line, col) / (basename, line); a suffix filter is
    applied at match time (any suffix match shares the basename, so bucketing on
    it is complete).
    """
    with path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames is None:
            raise SystemExit("error: cross-table CSV appears to be empty")
        fields = list(reader.fieldnames)
        c = {k: resolve_col(fields, v, where="cross table") for k, v in cols.items()}
        by_exact: dict[tuple, list[int]] = {}
        by_line: dict[tuple, list[int]] = {}
        appended: list[dict[str, str]] = []
        three_ids: list[tuple] = []
        files: list[str] = []
        for row in reader:
            fpath = (row.get(c["file"]) or "").strip()
            fbase = basename_of(fpath)
            line = to_int(row.get(c["line"]))
            col = to_int(row.get(c["begincol"]))
            idx = len(appended)
            app, tid = _cross_appended(row, c)
            appended.append(app)
            three_ids.append(tid)
            files.append(fpath)
            if fbase and line is not None:
                by_line.setdefault((fbase, line), []).append(idx)
                if col is not None:
                    by_exact.setdefault((fbase, line, col), []).append(idx)
    return by_exact, by_line, appended, three_ids, files


def _cross_appended(row: dict[str, str], c: dict[str, str]):
    match = (row.get(c["match"]) or "").strip()
    g_present = match in ("both", "genc2rust-only")
    t_present = match in ("both", "3c-only")
    name = (row.get(c["name"], "") or "") if t_present else ""
    app = {
        "match rule": row.get(c["match_rule"], "") or "",
        "genc2rust-match status": GENC_MATCH if g_present else GENC_NOMATCH,
        "genc2rust-classification": (row.get(c["classification"], "") or "")
        if g_present else GENC_MISSING,
        "3c-match status": THREEC_MATCH if t_present else THREEC_NOMATCH,
        "3c-classification": THREEC_WILD if t_present else THREEC_NOTWILD,
        "3c-reasons": (row.get(c["reasons"], "") or "") if t_present else "",
        "3c-ptrs affected": (row.get(c["ptrs_affected"], "") or "") if t_present else "",
        "3c-ptrs score": (row.get(c["ptrs_score"], "") or "") if t_present else "",
        "3c-name": name,
        # "name-match" filled per classification row at join time
    }
    tid = name_identity_3c(name) if t_present else (None, False)
    return app, tid


def join_one(class_path: Path, out_path: Path, key_cols: dict[str, str],
             index, *, label: str) -> None:
    by_exact, by_line, appended, three_ids, files = index
    with class_path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames is None:
            raise SystemExit(f"error: {label} CSV appears to be empty")
        left_fields = list(reader.fieldnames)
        kc = {k: resolve_col(left_fields, v, where=label) for k, v in key_cols.items()}
        out_fields = left_fields + APPENDED

        out_path.parent.mkdir(parents=True, exist_ok=True)
        n_in = n_out = n_exact = n_line = n_nomatch = n_mismatch = 0
        with out_path.open("w", newline="", encoding="utf-8") as ofh:
            writer = csv.DictWriter(ofh, fieldnames=out_fields)
            writer.writeheader()
            for row in reader:
                n_in += 1
                fpath = row.get(kc["file"], "") or ""
                fbase = basename_of(fpath)
                line = to_int(row.get(kc["line"]))
                col = to_int(row.get(kc["col"]))
                class_id = subject_identity(row.get(kc["name"], ""))

                hits: list[int] = []
                if fbase and line is not None and col is not None:
                    hits = [i for i in by_exact.get((fbase, line, col), [])
                            if file_suffix_match(fpath, files[i])]
                    if hits:
                        n_exact += 1
                if not hits and fbase and line is not None:
                    hits = [i for i in by_line.get((fbase, line), [])
                            if file_suffix_match(fpath, files[i])]
                    if hits:
                        n_line += 1

                if hits:
                    for idx in hits:
                        out = dict(row)
                        out.update(appended[idx])
                        nm = name_match(class_id, three_ids[idx])
                        out["name-match"] = nm
                        if nm == "mismatch":
                            n_mismatch += 1
                        writer.writerow(out)
                        n_out += 1
                else:
                    n_nomatch += 1
                    out = dict(row)
                    out.update(NOMATCH_APPENDED)
                    writer.writerow(out)
                    n_out += 1

    print(
        f"{label}: {n_in} rows in -> {n_out} out "
        f"(exact={n_exact}, line-fallback={n_line}, nomatch={n_nomatch}; "
        f"name-mismatch={n_mismatch}) => {out_path}",
        file=sys.stderr,
    )


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cross_csv", type=Path,
                    help="3C/GenC2Rust cross table (join-3c-genc2rust.py output)")
    ap.add_argument("--stable-classification", type=Path,
                    help="stable_classification CSV -> join-sc-3way.csv")
    ap.add_argument("--void-classification", type=Path,
                    help="void_classification CSV -> join-vc-3way.csv")
    ap.add_argument("--out", type=Path, required=True,
                    help="output directory for join-sc-3way.csv / join-vc-3way.csv")
    ap.add_argument("--cross-file-col", default="file")
    ap.add_argument("--cross-line-col", default="line")
    ap.add_argument("--cross-begincol-col", default="beginCol")
    ap.add_argument("--cross-match-col", default="match")
    ap.add_argument("--cross-matchrule-col", default="match_rule")
    ap.add_argument("--cross-classification-col", default="classification")
    ap.add_argument("--cross-reasons-col", default="Reasons")
    ap.add_argument("--cross-ptrsaffected-col", default="PtrsAffected")
    ap.add_argument("--cross-ptrsscore-col", default="PtrsScore")
    ap.add_argument("--cross-name-col", default="Name")
    ap.add_argument("--sc-file-col", default="defFile")
    ap.add_argument("--sc-line-col", default="defStartLine")
    ap.add_argument("--sc-col-col", default="defStartColumn")
    ap.add_argument("--sc-name-col", default="subject")
    ap.add_argument("--vc-file-col", default="source_file")
    ap.add_argument("--vc-line-col", default="source_line")
    ap.add_argument("--vc-col-col", default="source_column")
    ap.add_argument("--vc-name-col", default="declaration_name")
    args = ap.parse_args(argv)

    if not args.stable_classification and not args.void_classification:
        raise SystemExit(
            "error: provide --stable-classification and/or --void-classification")

    cross_cols = {
        "file": args.cross_file_col, "line": args.cross_line_col,
        "begincol": args.cross_begincol_col, "match": args.cross_match_col,
        "match_rule": args.cross_matchrule_col,
        "classification": args.cross_classification_col,
        "reasons": args.cross_reasons_col, "ptrs_affected": args.cross_ptrsaffected_col,
        "ptrs_score": args.cross_ptrsscore_col, "name": args.cross_name_col,
    }
    index = build_cross_index(args.cross_csv, cross_cols)
    print(f"cross table: {len(index[2])} rows indexed", file=sys.stderr)

    if args.stable_classification:
        join_one(
            args.stable_classification, args.out / "join-sc-3way.csv",
            {"file": args.sc_file_col, "line": args.sc_line_col,
             "col": args.sc_col_col, "name": args.sc_name_col},
            index, label="stable_classification",
        )
    if args.void_classification:
        join_one(
            args.void_classification, args.out / "join-vc-3way.csv",
            {"file": args.vc_file_col, "line": args.vc_line_col,
             "col": args.vc_col_col, "name": args.vc_name_col},
            index, label="void_classification",
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

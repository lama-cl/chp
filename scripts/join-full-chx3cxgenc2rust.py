#!/usr/bin/env python3
"""Full-join stable- and void-classification CSVs against the 3C/GenC2Rust cross
table (output of ``join-3c-genc2rust.py``, assumed to carry ``project``, ``file``,
``beginCol`` and ``Name``), producing two 3-way outputs.

Full join
---------
Every classification row is preserved, AND every cross-table row that no
classification row matched is emitted as its own output row with all
classification columns filled with the sentinel

    Not in cast history consideration

so 3C / GenC2Rust findings outside the cast-history universe stay visible. The
unmatched cross rows are emitted after the classification rows.

Which unmatched cross rows become sentinel rows is set by ``--tail-filter``:

    void (default)  rows with any void signal selected by ``--void-signals``
                    (default: all four):
                      genc2rust  GenC2Rust tracked it (GenC2Rust only tracks void*)
                      3c-void    cross ``3CVoidPtr == true`` (IsVoidPtr from a
                                 patched 3C dump, or the ``--void-reason``
                                 "Default void* type" constraint; legacy tables:
                                 that text in Reasons)
                      itype      3C solved it as an itype (3CClassification, or
                                 internal != external with a checked external)
                      generic    cross ``3CGeneric == true`` (patched 3C dump only)
                    Without the 3C dump patch, a 3C-only void* that 3C made a
                    plain generic (checked, internal == external) has no signal
                    and is left out. ``itype`` also admits non-void itypes
                    (e.g. an ``int *`` used unsafely inside a function).
    all             every unmatched cross row (with a solved-3C cross table this
                    includes every pointer 3C models, void or not).

"Unmatched" is computed independently per output: a cross row consumed by
join-sc-3way.csv may still be a sentinel row in join-vc-3way.csv.

Columns
-------
Leading coverage flags (``true``/``false``):

    InCastHistory   the row carries real cast-history classification data
    In3C            the row is backed by a 3C subject (checked, itype or wild)
    InGenC2Rust     the row is backed by a GenC2Rust finding

Then the classification columns, then:

    match rule                 <- cross table match_rule (blank if no match)
    genc2rust-match status      matched-genc2rust | nomatch-genc2rust
    genc2rust-classification    <- cross classification, or "not matched with genc2rust"
    3c-match status             match-3c | nomatch-3c
    3c-classification           <- cross 3CClassification (checked|itype|wild or
                                   wild|not wild, as chosen in join-3c-genc2rust.py);
                                   "not in 3C" when no 3C subject.
                                   Legacy cross tables without 3CClassification:
                                   wild if 3C present, else "not wild".
    3c-reasons                  <- cross Reasons        (blank if no 3c)
    3c-ptrs affected            <- cross PtrsAffected    (blank if no 3c)
    3c-ptrs score               <- cross PtrsScore       (blank if no 3c)
    3c-name                     <- cross 3C Name         (blank if no 3c)
    name-match                  match | mismatch | ""    (audit only, never filters)
    join-rule                   exact | return | line-only | "" (how THIS join paired)
    genc2rust-<col>             one per --genc2rust-extra-cols (default inferred_type,
                                causes = GenC2Rust's unsupported reason);
                                "not matched with genc2rust" when no GenC2Rust
    3c-kind, 3c-checked-kind, 3c-wild-origin, 3c-internal, 3c-external,
    3c-function, 3c-void-ptr, 3c-void-evidence, 3c-generic
                                <- cross 3C* columns (blank when absent / no 3c)

and finally the cross-row location audit columns, populated for every row
backed by a cross row (matched or sentinel), blank otherwise:

    cross-project, cross-file, cross-line, cross-col   unified cross location
    cross-3c-line, cross-3c-col                        3C's own location (3CLine/3CBeginCol)

A ``both`` return row's unified location is GenC2Rust's ``L:1``; the 3C columns
keep the function-name location.

Synthetic identity on sentinel rows
-----------------------------------
Each sentinel row gets a distinct identity in the dedup column (``declaration_id``
/ ``subject``; ``--vc-id-col`` / ``--sc-id-col``, empty string disables):

    not-in-cast-history:<cross project>:<cross file>:<cross line>:<cross col>

(``<cross project>:`` is omitted when the cross row has no project). The project
keeps ids distinct when several projects share a bare file name like ``main.c``.

Location matching (per classification row)
------------------------------------------
A cross row is a candidate only when BOTH hold:

* **file** -- trailing path-suffix match, directory components included
  (``long/dir/subdir/file.c`` matches ``subdir/file.c`` and ``file.c``;
  ``x/file.c`` does NOT match ``y/file.c``). A bare cross ``file`` such as
  ``main.c`` therefore matches any path ending in ``/main.c``.
* **project** (unless ``--ignore-project``, or the cross row has no project) --
  the classification row's project column (``--vc-project-col``, default
  ``project``; ``--sc-project-col``, default none) must equal the cross
  ``project``; when the row has no project column, the cross project must be a
  directory component of the row's path. So ``ht/a/main.c`` matches the cross
  row ``project=ht, file=main.c`` but not ``project=lil, file=main.c``.

1. **exact** -- file + begin ``(line, col)`` equals the cross row's unified
   location or its 3C location (both identifier-anchored for 3C subjects).
2. **return** -- a classification row whose kind column equals
   ``--return-kind-value`` (void_classification: ``declaration_kind == return``,
   located at the function name) with no exact hit pairs with the cross *return*
   rows on its line: ``3CKind == return``, a ``$ret`` 3C name, or on
   genc2rust-only rows a GenC2Rust return span (begin col 1 or multi-line).
3. **line-only** (``--line-fallback``) -- non-return rows only, against non-return
   cross rows on the same line: ``unique`` (default) only when there is exactly
   one candidate; ``all`` = every candidate (legacy); ``off``.

Fan-out: one output row per matched cross row; none -> one all-nomatch row.

name-match (audit)
------------------
Identity is the token after the last ``:`` (``AVTActionFlush::tmp`` -> ``tmp``);
the 3C name has ``>>``/``<<`` and ``_<id>`` stripped, and ``$ret`` is the return.
A classification row whose kind is the return kind is treated as the return.
Sentinel rows have no classification subject, so name-match is blank on them.

  * stable_classification keys on defFile / defStartLine / defStartColumn; identity from ``subject``
  * void_classification   keys on source_file / source_line / source_column;
                          identity from ``declaration_name``; kind from ``declaration_kind``

Outputs (into --out): join-sc-3way.csv, join-vc-3way.csv.

Usage examples
--------------
Void classification only, defaults (void tail, project-aware, unique fallback)::

    python3 join-classifications-3way.py cross.csv \\
        --void-classification void_classification.csv --out out/

Both classifications, every unmatched cross row in the tail::

    python3 join-classifications-3way.py cross.csv \\
        --stable-classification stable.csv --void-classification void.csv \\
        --out out/ --tail-filter all

Void tail from GenC2Rust and 3C void evidence only (no itype rows)::

    python3 join-classifications-3way.py cross.csv --void-classification void.csv \\
        --out out/ --void-signals genc2rust,3c-void

Legacy behaviour (old line fallback, file-only matching)::

    python3 join-classifications-3way.py cross.csv --void-classification void.csv \\
        --out out/ --line-fallback all --ignore-project

Different column names (e.g. a cross table with ``proj``/``path``)::

    python3 join-classifications-3way.py cross.csv --void-classification void.csv \\
        --out out/ --cross-file-col path --vc-project-col proj

Carry more GenC2Rust columns into the output::

    python3 join-classifications-3way.py cross.csv --void-classification void.csv \\
        --out out/ --genc2rust-extra-cols inferred_type,causes,end_line

Note: the cross ``project`` column is read as ``project`` (case-insensitive).
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

SENTINEL = "Not in cast history consideration"

COVERAGE = ["InCastHistory", "In3C", "InGenC2Rust"]

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
APPENDED_3C_NEW = [
    ("3c-kind", "3CKind"),
    ("3c-checked-kind", "3CCheckedKind"),
    ("3c-wild-origin", "3CWildOrigin"),
    ("3c-internal", "3CInternal"),
    ("3c-external", "3CExternal"),
    ("3c-function", "3CFunction"),
    ("3c-void-ptr", "3CVoidPtr"),
    ("3c-void-evidence", "3CVoidEvidence"),
    ("3c-generic", "3CGeneric"),
]
VOID_SIGNALS = ("genc2rust", "3c-void", "itype", "generic")
CROSS_AUDIT = ["cross-project", "cross-file", "cross-line", "cross-col",
               "cross-3c-line", "cross-3c-col"]

GENC_MATCH = "matched-genc2rust"
GENC_NOMATCH = "nomatch-genc2rust"
GENC_MISSING = "not matched with genc2rust"
THREEC_MATCH = "match-3c"
THREEC_NOMATCH = "nomatch-3c"
THREEC_WILD = "wild"
THREEC_NOTWILD = "not wild"
THREEC_ABSENT = "not in 3C"
DEFAULT_VOID_REASON = "Default void* type"

TRUE = "true"
FALSE = "false"

BLANK_CROSS_AUDIT = {k: "" for k in CROSS_AUDIT}

_ID_SUFFIX = re.compile(r"_\d+$")


# --------------------------------------------------------------------------- utils
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


def resolve_col_optional(fieldnames: list[str], logical: str | None) -> str | None:
    """Like resolve_col but returns None instead of exiting when absent."""
    if not logical:
        return None
    return {norm_header(f): f for f in fieldnames}.get(norm_header(logical))


def to_int(v: str | None) -> int | None:
    if v is None:
        return None
    try:
        return int(v.strip())
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


def subject_identity(value: str, *, is_return: bool = False) -> tuple[str | None, bool]:
    if is_return:
        return "return", True
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


def synthetic_id(project: str, file: str, line: str, col: str) -> str:
    if project:
        return f"not-in-cast-history:{project}:{file}:{line}:{col}"
    return f"not-in-cast-history:{file}:{line}:{col}"


def extra_out_name(col: str) -> str:
    return "genc2rust-" + col.strip().lower().replace("_", "-").replace(" ", "-")


# ---------------------------------------------------------------------- cross table
class CrossIndex:
    """Cross-table rows plus the location buckets used to match them.

    Each row is bucketed by (basename, line, col) / (basename, line) under its
    unified location AND its 3C location; a path-suffix filter is applied at
    match time. Rows without a usable file/line are retained -- they can never
    be matched, so the full join emits them as sentinel rows (tail filter
    permitting).
    """

    def __init__(self, path: Path, cols: dict[str, str], extra_cols: list[str],
                 void_reason: str, void_signals: set[str]) -> None:
        self.by_exact: dict[tuple, list[int]] = {}
        self.by_line: dict[tuple, list[int]] = {}
        self.appended: list[dict[str, str]] = []
        self.audit: list[dict[str, str]] = []
        self.three_ids: list[tuple] = []
        self.files: list[str] = []
        self.is_ret: list[bool] = []
        self.is_void: list[bool] = []
        self.void_reason = void_reason
        self.void_signals = void_signals
        self.projects: list[str] = []

        with path.open("r", newline="", encoding="utf-8") as fh:
            reader = csv.DictReader(fh)
            if reader.fieldnames is None:
                raise SystemExit("error: cross-table CSV appears to be empty")
            fields = list(reader.fieldnames)
            req = ("file", "line", "begincol", "match", "match_rule",
                   "classification", "reasons", "ptrs_affected", "ptrs_score", "name")
            c: dict[str, str | None] = {
                k: resolve_col(fields, cols[k], where="cross table") for k in req}
            for k in ("endline", "3c_class", "3c_line", "3c_col", "3c_kind"):
                c[k] = resolve_col_optional(fields, cols[k])
            self.c = c
            self.project_col = resolve_col_optional(fields, "project")
            self.new3c = {out: resolve_col_optional(fields, src)
                          for out, src in APPENDED_3C_NEW}
            self.extras = {extra_out_name(e): resolve_col_optional(fields, e)
                           for e in extra_cols}
            missing = [e for e, col in zip(extra_cols, self.extras.values()) if col is None]
            if missing:
                print(f"warning: cross table lacks GenC2Rust column(s) {missing}; "
                      f"their output columns stay blank", file=sys.stderr)
            self.legacy = c["3c_class"] is None
            if self.legacy:
                print("note: cross table has no 3CClassification column -> legacy "
                      "presence rule (3C present = wild)", file=sys.stderr)

            for row in reader:
                i = len(self.appended)
                fpath = (row.get(c["file"]) or "").strip()
                fbase = basename_of(fpath)
                app, tid, ret, void = self._appended(row)
                self.appended.append(app)
                self.three_ids.append(tid)
                self.files.append(fpath)
                self.projects.append((row.get(self.project_col, "") or "").strip()
                                     if self.project_col else "")
                self.is_ret.append(ret)
                self.is_void.append(void)
                l3 = (row.get(c["3c_line"]) or "").strip() if c["3c_line"] else ""
                c3 = (row.get(c["3c_col"]) or "").strip() if c["3c_col"] else ""
                t_present = app["3c-match status"] == THREEC_MATCH
                self.audit.append({
                    "cross-project": (row.get(self.project_col, "") or "")
                    if self.project_col else "",
                    "cross-file": fpath,
                    "cross-line": (row.get(c["line"], "") or "").strip(),
                    "cross-col": (row.get(c["begincol"], "") or "").strip(),
                    "cross-3c-line": l3 if t_present else "",
                    "cross-3c-col": c3 if t_present else "",
                })
                if not fbase:
                    continue
                locs = {(to_int(row.get(c["line"])), to_int(row.get(c["begincol"])))}
                if t_present and l3:
                    locs.add((to_int(l3), to_int(c3)))
                lines = set()
                for ln, col in locs:
                    if ln is None:
                        continue
                    lines.add(ln)
                    if col is not None:
                        self.by_exact.setdefault((fbase, ln, col), []).append(i)
                for ln in lines:
                    self.by_line.setdefault((fbase, ln), []).append(i)

    def __len__(self) -> int:
        return len(self.appended)

    def _appended(self, row: dict[str, str]):
        c = self.c
        match = (row.get(c["match"]) or "").strip()
        g_present = match in ("both", "genc2rust-only")
        t_present = match in ("both", "3c-only")
        name = (row.get(c["name"], "") or "") if t_present else ""
        reasons = (row.get(c["reasons"], "") or "") if t_present else ""
        if self.legacy:
            cls = THREEC_WILD if t_present else THREEC_NOTWILD
        else:
            cls = (row.get(c["3c_class"]) or "") if t_present else THREEC_ABSENT
        app = {
            "match rule": row.get(c["match_rule"], "") or "",
            "genc2rust-match status": GENC_MATCH if g_present else GENC_NOMATCH,
            "genc2rust-classification": (row.get(c["classification"], "") or "")
            if g_present else GENC_MISSING,
            "3c-match status": THREEC_MATCH if t_present else THREEC_NOMATCH,
            "3c-classification": cls,
            "3c-reasons": reasons,
            "3c-ptrs affected": (row.get(c["ptrs_affected"], "") or "") if t_present else "",
            "3c-ptrs score": (row.get(c["ptrs_score"], "") or "") if t_present else "",
            "3c-name": name,
            # "name-match" / "join-rule" filled per classification row at join time
        }
        for out, src in self.extras.items():
            if not g_present:
                app[out] = GENC_MISSING
            else:
                app[out] = (row.get(src, "") or "") if src else ""
        for out, src in self.new3c.items():
            app[out] = (row.get(src, "") or "") if (src and t_present) else ""

        # is this cross row a return?
        kind = (row.get(c["3c_kind"]) or "") if (c["3c_kind"] and t_present) else ""
        if kind:
            is_ret = kind == "return"
        elif t_present:
            is_ret = "$ret" in name
        else:
            bl, bc = to_int(row.get(c["line"])), to_int(row.get(c["begincol"]))
            el = to_int(row.get(c["endline"])) if c["endline"] else None
            is_ret = bl is not None and (bc == 1 or (el is not None and el != bl))
        # void pointer evidence (for --tail-filter void), per --void-signals
        sig = self.void_signals
        vcol = self.new3c.get("3c-void-ptr")
        void3c = (row.get(vcol) or "").strip().lower() if (vcol and t_present) else ""
        if t_present and not void3c and self.void_reason in reasons:
            void3c = "true"                      # legacy / pre-3CVoidPtr cross tables
        gcol = self.new3c.get("3c-generic")
        generic = (row.get(gcol) or "").strip().lower() if (gcol and t_present) else ""
        is_itype = t_present and cls == "itype"
        if t_present and not is_itype and self.new3c.get("3c-internal") \
                and self.new3c.get("3c-external"):
            il = (row.get(self.new3c["3c-internal"]) or "")
            el = (row.get(self.new3c["3c-external"]) or "")
            is_itype = bool(il and el and il != el and not el.startswith("WILD"))
        is_void = (("genc2rust" in sig and g_present)
                   or ("3c-void" in sig and void3c == "true")
                   or ("itype" in sig and is_itype)
                   or ("generic" in sig and generic == "true"))
        tid = name_identity_3c(name) if t_present else (None, False)
        return app, tid, is_ret, is_void


def coverage_of(app: dict[str, str], *, in_cast_history: bool) -> dict[str, str]:
    return {
        "InCastHistory": TRUE if in_cast_history else FALSE,
        "In3C": TRUE if app["3c-match status"] == THREEC_MATCH else FALSE,
        "InGenC2Rust": TRUE if app["genc2rust-match status"] == GENC_MATCH else FALSE,
    }


# ----------------------------------------------------------------------------- join
def project_ok(class_project: str, class_path: str, cross_project: str) -> bool:
    """Project check: equal project values when the classification row has one;
    otherwise the cross project must be a directory component of the path."""
    if not cross_project:
        return True
    if class_project:
        return class_project == cross_project
    return cross_project in path_components(class_path)[:-1]


def join_one(class_path: Path, out_path: Path, key_cols: dict[str, str | None],
             index: CrossIndex, *, label: str, id_col: str | None,
             line_fallback: str, return_kind: str, tail_filter: str,
             use_project: bool) -> None:
    extra_names = list(index.extras)
    tail_cols = ["join-rule"] + extra_names + [o for o, _ in APPENDED_3C_NEW]
    nomatch_appended = {
        "match rule": "", "genc2rust-match status": GENC_NOMATCH,
        "genc2rust-classification": GENC_MISSING, "3c-match status": THREEC_NOMATCH,
        "3c-classification": THREEC_NOTWILD if index.legacy else THREEC_ABSENT,
        "3c-reasons": "", "3c-ptrs affected": "", "3c-ptrs score": "", "3c-name": "",
        "name-match": "", "join-rule": "",
        **{e: GENC_MISSING for e in extra_names},
        **{o: "" for o, _ in APPENDED_3C_NEW},
    }

    with class_path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames is None:
            raise SystemExit(f"error: {label} CSV appears to be empty")
        left_fields = list(reader.fieldnames)
        kc = {k: resolve_col(left_fields, v, where=label)
              for k, v in key_cols.items() if k not in ("kind", "project")}
        kind_col = resolve_col_optional(left_fields, key_cols.get("kind"))
        proj_col = resolve_col_optional(left_fields, key_cols.get("project")) \
            if use_project else None
        if key_cols.get("kind") and not kind_col:
            print(f"warning: {label} has no kind column '{key_cols['kind']}'; "
                  f"return-aware matching disabled", file=sys.stderr)
        resolved_id_col = resolve_col(left_fields, id_col, where=label) if id_col else None
        out_fields = COVERAGE + left_fields + APPENDED + tail_cols + CROSS_AUDIT

        matched_cross: set[int] = set()
        cnt = dict.fromkeys(("in", "out", "exact", "return", "line", "nomatch",
                             "mismatch", "tail", "tail_skipped"), 0)

        out_path.parent.mkdir(parents=True, exist_ok=True)
        with out_path.open("w", newline="", encoding="utf-8") as ofh:
            writer = csv.DictWriter(ofh, fieldnames=out_fields)
            writer.writeheader()

            for row in reader:
                cnt["in"] += 1
                fpath = row.get(kc["file"], "") or ""
                fbase = basename_of(fpath)
                line = to_int(row.get(kc["line"]))
                col = to_int(row.get(kc["col"]))
                is_ret = bool(kind_col) and \
                    (row.get(kind_col) or "").strip().lower() == return_kind.lower()
                class_id = subject_identity(row.get(kc["name"], ""), is_return=is_ret)

                cproj = (row.get(proj_col) or "").strip() if proj_col else ""

                def same_file(ids: list[int]) -> list[int]:
                    return [i for i in ids
                            if file_suffix_match(fpath, index.files[i])
                            and (not use_project
                                 or project_ok(cproj, fpath, index.projects[i]))]

                hits: list[int] = []
                rule = ""
                if fbase and line is not None and col is not None:
                    hits = same_file(index.by_exact.get((fbase, line, col), []))
                    rule = "exact" if hits else ""
                if not hits and fbase and line is not None:
                    on_line = same_file(index.by_line.get((fbase, line), []))
                    if is_ret:
                        hits = [i for i in on_line if index.is_ret[i]]
                        rule = "return" if hits else ""
                    elif line_fallback != "off":
                        cands = [i for i in on_line if not index.is_ret[i]]
                        if line_fallback == "all" or len(cands) == 1:
                            hits = cands
                            rule = "line-only" if hits else ""
                hits = list(dict.fromkeys(hits))

                if hits:
                    cnt[{"exact": "exact", "return": "return", "line-only": "line"}[rule]] += 1
                    for i in hits:
                        matched_cross.add(i)
                        app = index.appended[i]
                        out = dict(row)
                        out.update(app)
                        nm = name_match(class_id, index.three_ids[i])
                        out["name-match"] = nm
                        out["join-rule"] = rule
                        if nm == "mismatch":
                            cnt["mismatch"] += 1
                        out.update(index.audit[i])
                        out.update(coverage_of(app, in_cast_history=True))
                        writer.writerow(out)
                        cnt["out"] += 1
                else:
                    cnt["nomatch"] += 1
                    out = dict(row)
                    out.update(nomatch_appended)
                    out.update(BLANK_CROSS_AUDIT)
                    out.update(coverage_of(nomatch_appended, in_cast_history=True))
                    writer.writerow(out)
                    cnt["out"] += 1

            # Full-join tail: cross rows no classification row claimed.
            for i in range(len(index)):
                if i in matched_cross:
                    continue
                if tail_filter == "void" and not index.is_void[i]:
                    cnt["tail_skipped"] += 1
                    continue
                cnt["tail"] += 1
                app = index.appended[i]
                audit = index.audit[i]
                out = {f: SENTINEL for f in left_fields}
                if resolved_id_col:
                    out[resolved_id_col] = synthetic_id(
                        audit["cross-project"], audit["cross-file"],
                        audit["cross-line"], audit["cross-col"])
                out.update(app)
                out["name-match"] = ""
                out["join-rule"] = ""
                out.update(audit)
                out.update(coverage_of(app, in_cast_history=False))
                writer.writerow(out)
                cnt["out"] += 1

    print(
        f"{label}: {cnt['in']} rows in -> {cnt['out']} out "
        f"(exact={cnt['exact']}, return={cnt['return']}, line-fallback={cnt['line']}, "
        f"nomatch={cnt['nomatch']}; name-mismatch={cnt['mismatch']}; "
        f"cross-only={cnt['tail']} emitted, {cnt['tail_skipped']} skipped by "
        f"--tail-filter {tail_filter}, of {len(index)}) => {out_path}",
        file=sys.stderr,
    )


# ----------------------------------------------------------------------------- main
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
    ap.add_argument("--tail-filter", choices=["void", "all"], default="void",
                    help="unmatched cross rows emitted as sentinel rows: void "
                         "(GenC2Rust-tracked or 3C void* reason) or all")
    ap.add_argument("--void-signals", default="genc2rust,3c-void,itype,generic",
                    help="comma list of what counts as void for --tail-filter void: "
                         + ",".join(VOID_SIGNALS))
    ap.add_argument("--ignore-project", action="store_true",
                    help="match on file path only (legacy); by default the cross "
                         "project must match the row's project column, or appear "
                         "as a directory in its path")
    ap.add_argument("--void-reason", default=DEFAULT_VOID_REASON,
                    help="3C reason text marking a void* (used by --tail-filter void)")
    ap.add_argument("--line-fallback", choices=["unique", "all", "off"], default="unique",
                    help="line-only pairing for non-return rows (legacy behaviour: all)")
    ap.add_argument("--return-kind-value", default="return",
                    help="kind-column value marking a return row")
    ap.add_argument("--genc2rust-extra-cols", default="inferred_type,causes",
                    help="comma-separated GenC2Rust cross columns carried as genc2rust-<col>")
    ap.add_argument("--cross-file-col", default="file")
    ap.add_argument("--cross-line-col", default="line")
    ap.add_argument("--cross-begincol-col", default="beginCol")
    ap.add_argument("--cross-endline-col", default="endLine")
    ap.add_argument("--cross-match-col", default="match")
    ap.add_argument("--cross-matchrule-col", default="match_rule")
    ap.add_argument("--cross-classification-col", default="classification")
    ap.add_argument("--cross-reasons-col", default="Reasons")
    ap.add_argument("--cross-ptrsaffected-col", default="PtrsAffected")
    ap.add_argument("--cross-ptrsscore-col", default="PtrsScore")
    ap.add_argument("--cross-name-col", default="Name")
    ap.add_argument("--cross-3cclass-col", default="3CClassification")
    ap.add_argument("--cross-3cline-col", default="3CLine")
    ap.add_argument("--cross-3ccol-col", default="3CBeginCol")
    ap.add_argument("--cross-3ckind-col", default="3CKind")
    ap.add_argument("--sc-file-col", default="defFile")
    ap.add_argument("--sc-line-col", default="defStartLine")
    ap.add_argument("--sc-col-col", default="defStartColumn")
    ap.add_argument("--sc-name-col", default="subject")
    ap.add_argument("--sc-kind-col", default="",
                    help="stable_classification kind column (none by default)")
    ap.add_argument("--sc-project-col", default="",
                    help="stable_classification project column (none by default: "
                         "project is looked up in the defFile path)")
    ap.add_argument("--sc-id-col", default="subject",
                    help="column given a synthetic id on cross-only rows "
                         "(empty string to disable)")
    ap.add_argument("--vc-file-col", default="source_file")
    ap.add_argument("--vc-line-col", default="source_line")
    ap.add_argument("--vc-col-col", default="source_column")
    ap.add_argument("--vc-name-col", default="declaration_name")
    ap.add_argument("--vc-kind-col", default="declaration_kind")
    ap.add_argument("--vc-project-col", default="project")
    ap.add_argument("--vc-id-col", default="declaration_id",
                    help="column given a synthetic id on cross-only rows "
                         "(empty string to disable)")
    args = ap.parse_args(argv)

    if not args.stable_classification and not args.void_classification:
        raise SystemExit(
            "error: provide --stable-classification and/or --void-classification")

    cross_cols = {
        "file": args.cross_file_col, "line": args.cross_line_col,
        "begincol": args.cross_begincol_col, "endline": args.cross_endline_col,
        "match": args.cross_match_col, "match_rule": args.cross_matchrule_col,
        "classification": args.cross_classification_col,
        "reasons": args.cross_reasons_col, "ptrs_affected": args.cross_ptrsaffected_col,
        "ptrs_score": args.cross_ptrsscore_col, "name": args.cross_name_col,
        "3c_class": args.cross_3cclass_col, "3c_line": args.cross_3cline_col,
        "3c_col": args.cross_3ccol_col, "3c_kind": args.cross_3ckind_col,
    }
    extra_cols = [e.strip() for e in args.genc2rust_extra_cols.split(",") if e.strip()]
    signals = {s.strip() for s in args.void_signals.split(",") if s.strip()}
    bad = signals - set(VOID_SIGNALS)
    if bad:
        raise SystemExit(f"error: unknown --void-signals {sorted(bad)}; "
                         f"choose from {', '.join(VOID_SIGNALS)}")
    index = CrossIndex(args.cross_csv, cross_cols, extra_cols, args.void_reason, signals)
    if not index.project_col and not args.ignore_project:
        print("warning: cross table has no 'project' column; matching on file only",
              file=sys.stderr)
    print(f"cross table: {len(index)} rows indexed", file=sys.stderr)

    common = dict(line_fallback=args.line_fallback, return_kind=args.return_kind_value,
                  tail_filter=args.tail_filter, use_project=not args.ignore_project)
    if args.stable_classification:
        join_one(
            args.stable_classification, args.out / "join-sc-3way.csv",
            {"file": args.sc_file_col, "line": args.sc_line_col,
             "col": args.sc_col_col, "name": args.sc_name_col,
             "kind": args.sc_kind_col or None, "project": args.sc_project_col or None},
            index, label="stable_classification", id_col=args.sc_id_col or None,
            **common,
        )
    if args.void_classification:
        join_one(
            args.void_classification, args.out / "join-vc-3way.csv",
            {"file": args.vc_file_col, "line": args.vc_line_col,
             "col": args.vc_col_col, "name": args.vc_name_col,
             "kind": args.vc_kind_col or None, "project": args.vc_project_col or None},
            index, label="void_classification", id_col=args.vc_id_col or None,
            **common,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

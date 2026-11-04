
#!/usr/bin/env python3
"""Full-outer-join 3C's *solved* constraint output with the GenC2Rust taints CSV.

3C input
--------
``<prefix>_final_output.json`` written by
``3c --addcr --alltypes -dump-intermediate -constraint-output=<prefix> ...``
(dumped right after constraint solving). Every pointer 3C models is present,
checked or wild, so the 3C classification is read from the solved
``Setup.Environment`` rather than inferred from presence in PerWildPtrStats.

A **3C subject** is one located declaration:

* ``var``    -- a ``PointerVar`` entry (local, global, field). Location = the
                declaration identifier (``NamedDecl::getLocation()``).
* ``param``  -- a ``PointerVar`` entry whose atoms are a function parameter's
                *internal* variable (3C stores ``Variables[param PSL] =
                internal``). Location = the parameter identifier. Its
                *external* variable is taken from the owning function's
                ``FunctionVar.Parameters[i] = [internal, external]`` pair
                (3C dumps internal first, then external).
* ``return`` -- the ``ReturnVar = [internal, external]`` of a ``FunctionVar``
                entry. Location = the function *name* identifier. Skipped when
                the return is not a pointer (``Vars: []``).

Entries at ``:0:0:0`` (no source location, e.g. implicit builtins) are
null-location subjects.

Classification (outer pointer level; mirrors 3C's own rewriting rule)
---------------------------------------------------------------------
* external outer level solves to ``WILD``                 -> ``wild``
* external checked and internal levels == external levels -> ``checked``
* external checked, internal differs (internal unchecked) -> ``itype``

For non-function subjects external == internal. For a plain ``void *``
parameter/return without a source itype, 3C uses ONE variable for both sides,
so ``itype`` cannot occur there.

``--3c-label-mode tri``    -> checked | itype | wild
``--3c-label-mode binary`` -> wild | not wild  (itype -> ``--binary-itype``)

Wild origin: ``direct`` when the wild atom has a checked ``Geq(atom, WILD)``
constraint (its reasons are reported, e.g. ``Default void* type``), otherwise
``indirect`` (wild by flow). ``--trace-indirect-roots`` additionally names the
root cause(s) of indirect wild atoms (slower on big programs).

Optional ``--3c-root-causes PerWildPtrStats.json`` (from ``-dump-stats`` in the
SAME 3C run) fills PtrsAffected / PtrsScore / InSrc / SubReasons by atom name.

Matching (per GenC2Rust row; source file is not in the key, as before)
----------------------------------------------------------------------
A GenC2Rust row is a **return** when ``begin col == 1`` or it spans lines.

* Return     -> ``return`` subjects whose function name is on the row's begin
                line. Tag ``return``. If none, the nearest function-name line in
                ``(begin, begin + --return-nearby]``. Tag ``return-nearby``.
* Non-return -> ``var``/``param`` subjects at the exact ``(line, col)``.
                Tag ``exact``. Otherwise ``--line-fallback``:
                ``unique`` (default) pairs with the single non-return subject on
                that line whose column no GenC2Rust row on the line occupies;
                ``all`` pairs with every such subject; ``off`` disables it.
                Tag ``line-only``.

All matching pairs are emitted (fan-out); ``match_rule`` records how.

Full outer join
---------------
* GenC2Rust rows with no 3C subject -> 3C columns = ``--3c-sentinel``.
* 3C subjects with no GenC2Rust row  -> GenC2Rust columns =
  ``"not considered by genc2rust"``.

Output columns (existing columns unchanged; new 3C columns appended)
--------------------------------------------------------------------
    match, match_rule, line, beginCol, endLine, endCol
    <all GenC2Rust columns verbatim>
    Name, Reasons, PtrsAffected, PtrsScore, InSrc, SubReasons
    3CClassification   checked | itype | wild   (or wild | not wild)
    3CCheckedKind      PTR | ARR | NTARR of the advertised (external) outer level; blank if wild
    3CWildOrigin       direct | indirect | ""
    3CKind             var | param | return
    3CInternal         solved checked value per level, internal side ("ARR|NTARR")
    3CExternal         same, external side (== internal for var)
    3CAtoms            internal atoms [ / external atoms when different ]
    3CFunction         owning function for param/return
    3CVoidPtr          true | false | "" (unknown) -- is this a void pointer?
    3CVoidEvidence     json | json-basetype | reason | ""
    3CGeneric          true | false | "" -- 3C made it a generic (_For_any/_Itype_for_any)
    3CFile, 3CLine, 3CBeginCol, 3CEndCol   the 3C subject's own location

``line``/``beginCol`` stay the unified location (GenC2Rust's on ``both`` rows),
so a ``both`` return row carries GenC2Rust's ``L:1``; ``3CLine/3CBeginCol``
carry 3C's function-name location for downstream joins.

Void-pointer evidence (3CVoidPtr)
---------------------------------
final_output.json carries no pointer types, so void-ness is inferred:

* ``json``          -- the PointerVar carries ``IsVoidPtr`` (requires the small 3C
                       dumper patch below); authoritative, incl. generic void*.
* ``json-basetype`` -- the PointerVar carries ``BaseType`` (``void``/``const void``).
* ``reason``        -- any of the subject's atoms has a checked
                       ``Geq(atom, WILD)`` with ``--void-reason``
                       (default ``Default void* type``): every NON-generic void*.
* ""                -- unknown: a void* 3C turned generic (checked, no reason) is
                       indistinguishable from a non-void pointer without the patch.

3C dumper patch (clang/lib/3C/ConstraintVariables.cpp,
``PointerVariableConstraint::dumpJson``), after the ``"name"`` field::

    O << ", \\"BaseType\\":" << llvm::json::Value(BaseType)
      << ", \\"IsVoidPtr\\":" << (IsVoidPtr ? "true" : "false")
      << ", \\"GenericIndex\\":" << InferredGenericIndex;

Usage examples
--------------
Produce the 3C input (same run for both JSON files)::

    3c --addcr --alltypes -dump-intermediate -constraint-output=out \\
       -dump-stats -perptrstats-output=PerWildPtrStats.json \\
       -output-dir=out-3c main.c
    # -> out_final_output.json, PerWildPtrStats.json

Basic join (tri labels: checked | itype | wild) to stdout::

    python3 join-3c-genc2rust.py out_final_output.json genc2rust-taints.csv

Binary labels, itypes counted as not wild (paper convention), to a file::

    python3 join-3c-genc2rust.py out_final_output.json taints.csv \\
        --3c-label-mode binary --binary-itype not-wild --out cross.csv

Add 3C root-cause stats and name root causes of indirectly wild pointers::

    python3 join-3c-genc2rust.py out_final_output.json taints.csv --out cross.csv \\
        --3c-root-causes PerWildPtrStats.json --trace-indirect-roots

3C run covering headers too -- keep only one source file's subjects::

    python3 join-3c-genc2rust.py out_final_output.json taints.csv --out cross.csv \\
        --3c-file air-connect/main.c

Strict matching (no line-only pairing, no return-nearby search)::

    python3 join-3c-genc2rust.py out_final_output.json taints.csv --out cross.csv \\
        --line-fallback off --return-nearby 0

Per project (the 3-way join expects ``project`` and ``file`` columns)::

    for p in ht brotli air-connect; do
      python3 join-3c-genc2rust.py 3c/$p/out_final_output.json gc/$p/taints.csv \\
          --out cross/$p.csv
    done
    # then prepend project,file columns and concatenate into cross.csv
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from collections import Counter, deque
from pathlib import Path
from typing import Any

GENC_SENTINEL = "not considered by genc2rust"
DEFAULT_3C_SENTINEL = "Not in 3C output"

UNIFIED_COLS = ["line", "beginCol", "endLine", "endCol"]
THREEC_COLS = ["Name", "Reasons", "PtrsAffected", "PtrsScore", "InSrc", "SubReasons"]
THREEC_NEW_COLS = [
    "3CClassification", "3CCheckedKind", "3CWildOrigin", "3CKind",
    "3CInternal", "3CExternal", "3CAtoms", "3CFunction",
    "3CVoidPtr", "3CVoidEvidence", "3CGeneric",
    "3CFile", "3CLine", "3CBeginCol", "3CEndCol",
]
CONST_ATOMS = {"PTR", "ARR", "NTARR", "WILD"}
LINK_REASON = "Function Internal/External Link"

_PSL_RE = re.compile(r"^(?P<path>.*):(?P<line>\d+):(?P<scol>\d+):(?P<ecol>\d+)$")


# --------------------------------------------------------------------------- utils
def norm_header(name: str) -> str:
    return name.strip().lower().replace(" ", "_").replace("-", "_")


def resolve_col(fieldnames: list[str], logical: str) -> str:
    want = norm_header(logical)
    lookup = {norm_header(f): f for f in fieldnames}
    if want not in lookup:
        raise SystemExit(
            f"error: GenC2Rust CSV has no column matching '{logical}'. "
            f"Available columns: {', '.join(fieldnames)}"
        )
    return lookup[want]


def path_components(p: str) -> list[str]:
    return [c for c in re.split(r"[/\\]", (p or "").strip()) if c not in ("", ".")]


def file_suffix_match(a: str, b: str) -> bool:
    ca, cb = path_components(a), path_components(b)
    if not ca or not cb:
        return False
    n = min(len(ca), len(cb))
    return ca[-n:] == cb[-n:]


def parse_psl(s: str) -> tuple[str, int, int, int] | None:
    m = _PSL_RE.match(s or "")
    if not m:
        return None
    path, line = m.group("path"), int(m.group("line"))
    if line == 0 or not path:
        return None
    return path, line, int(m.group("scol")), int(m.group("ecol"))


def pv_atoms(var: Any) -> list[str]:
    """Atom names of a {"PointerVar": {...}} object (outer level first)."""
    if isinstance(var, dict) and "PointerVar" in var:
        return [str(a) for a in var["PointerVar"].get("Vars", [])]
    return []


def pv_obj(var: Any) -> dict:
    return var["PointerVar"] if isinstance(var, dict) and "PointerVar" in var else {}


def void_evidence(tc: "ThreeC", pvs: list[dict], atoms: list[str]) -> tuple[str, str, str]:
    """(3CVoidPtr, 3CVoidEvidence, 3CGeneric) for one subject.

    Uses ``IsVoidPtr`` / ``BaseType`` / ``GenericIndex`` when the 3C JSON carries
    them (patched dumper), else the ``--void-reason`` constraint on any atom.
    """
    is_void, evidence, generic = "", "", ""
    for pv in pvs:
        if "IsVoidPtr" in pv:
            is_void = "true" if pv["IsVoidPtr"] in (True, "true", 1) else \
                (is_void or "false")
            evidence = "json"
        elif "BaseType" in pv and not evidence:
            is_void = "true" if str(pv["BaseType"]).strip().endswith("void") else "false"
            evidence = "json-basetype"
        if "GenericIndex" in pv:
            try:
                g = int(pv["GenericIndex"]) >= 0
            except (TypeError, ValueError):
                g = False
            generic = "true" if (g or generic == "true") else "false"
    if is_void != "true" and any(tc.void_reason in tc.direct.get(a, []) for a in atoms):
        is_void, evidence = "true", "reason"
    return is_void, evidence, generic


# --------------------------------------------------------------------- 3C model
class ThreeC:
    def __init__(self, data: dict[str, Any], *, trace_roots: bool,
                 void_reason: str = "Default void* type") -> None:
        self.void_reason = void_reason
        setup = data.get("Setup", {})
        self.env: dict[str, tuple[str, str]] = {}
        for e in setup.get("Environment", []):
            val = e.get("value:", e.get("value", {})) or {}
            self.env[str(e.get("var"))] = (str(val.get("checked", "")),
                                          str(val.get("PtrType", "")))
        # direct wild reasons + checked flow graph (Geq(A1, A2): A2 wild => A1 wild)
        self.direct: dict[str, list[str]] = {}
        self.flow: dict[str, list[str]] = {}
        self.links: set[frozenset] = set()
        for c in setup.get("Constraints", []):
            g = c.get("Geq") if isinstance(c, dict) else None
            if not g:
                continue
            a1, a2 = str(g.get("Atom1")), str(g.get("Atom2"))
            reason = str(g.get("Reason", ""))
            if reason == LINK_REASON:
                self.links.add(frozenset((a1, a2)))
            if not g.get("isChecked"):
                continue
            if a2 == "WILD" and a1 not in CONST_ATOMS:
                lst = self.direct.setdefault(a1, [])
                if reason not in lst:
                    lst.append(reason)
            elif a1 not in CONST_ATOMS and a2 not in CONST_ATOMS:
                self.flow.setdefault(a2, []).append(a1)
        self.roots_of: dict[str, list[str]] = {}
        if trace_roots:
            self._trace_roots()

    def _trace_roots(self, cap: int = 3) -> None:
        for root in self.direct:
            seen = {root}
            q = deque([root])
            while q:
                a = q.popleft()
                for b in self.flow.get(a, []):
                    if b in seen:
                        continue
                    seen.add(b)
                    q.append(b)
                    if b not in self.direct:
                        lst = self.roots_of.setdefault(b, [])
                        if len(lst) < cap:
                            lst.append(root)

    def checked(self, atom: str) -> str:
        if atom in CONST_ATOMS:
            return atom
        return self.env.get(atom, ("?", "?"))[0]


def build_subjects(data: dict[str, Any], tc: ThreeC, file_filter: str | None):
    """Return (located, null_located, stats)."""
    entries = data.get("ConstraintVariables", [])
    # param internal outer atom -> (function, index, internal atoms, external atoms)
    param_map: dict[str, tuple[str, int, list[str], list[str], dict, dict]] = {}
    fv_sources: list[dict] = []
    for ent in entries:
        for v in ent.get("Variables", []):
            if isinstance(v, dict) and "FunctionVar" in v:
                fv_sources.append(v["FunctionVar"])
    for key in ("ExternalFunctionDefinitions", "StaticFunctionDefinitions"):
        for d in data.get(key, []) or []:
            for v in d.get("Constraints", []) or []:
                if isinstance(v, dict) and "FunctionVar" in v:
                    fv_sources.append(v["FunctionVar"])
    for fv in fv_sources:
        fname = str(fv.get("name", ""))
        for i, pair in enumerate(fv.get("Parameters", []) or []):
            if not isinstance(pair, list) or len(pair) != 2:
                continue
            ia, ea = pv_atoms(pair[0]), pv_atoms(pair[1])
            if ia and ia[0] not in CONST_ATOMS:
                param_map.setdefault(ia[0], (fname, i, ia, ea,
                                             pv_obj(pair[0]), pv_obj(pair[1])))

    stats = Counter()
    seen: set[tuple] = set()
    located: list[dict] = []
    null_located: list[dict] = []

    def add(loc, kind, func, ia, ea, pvs):
        key = (loc, kind, tuple(ia), tuple(ea))
        if key in seen:
            stats["duplicate"] += 1
            return
        seen.add(key)
        rec = classify(tc, kind, func, ia, ea)
        rec["loc"] = loc
        (rec["3CVoidPtr"], rec["3CVoidEvidence"],
         rec["3CGeneric"]) = void_evidence(tc, pvs, ia + ea)
        (located if loc else null_located).append(rec)
        stats[kind] += 1

    for ent in entries:
        loc = parse_psl(str(ent.get("line", "")))
        if loc and file_filter and not file_suffix_match(loc[0], file_filter):
            stats["filtered-file"] += 1
            continue
        for v in ent.get("Variables", []):
            if not isinstance(v, dict):
                continue
            if "PointerVar" in v:
                ia = pv_atoms(v)
                if not ia:
                    stats["non-pointer"] += 1
                    continue
                pm = param_map.get(ia[0])
                if pm and pm[2] == ia:
                    add(loc, "param", pm[0], ia, pm[3] or ia,
                        [pv_obj(v), pm[4], pm[5]])
                else:
                    add(loc, "var", "", ia, ia, [pv_obj(v)])
            elif "FunctionVar" in v:
                fv = v["FunctionVar"]
                rv = fv.get("ReturnVar", []) or []
                ia = pv_atoms(rv[0]) if len(rv) > 0 else []
                ea = pv_atoms(rv[1]) if len(rv) > 1 else []
                if not ia:
                    stats["non-pointer-return"] += 1
                    continue
                add(loc, "return", str(fv.get("name", "")), ia, ea or ia,
                    [pv_obj(r) for r in rv[:2]])

    # sanity: internal/external pairing vs "Function Internal/External Link"
    for rec in located + null_located:
        ia, ea = rec["_ia"], rec["_ea"]
        if rec["3CKind"] in ("param", "return") and ia != ea:
            if frozenset((ia[0], ea[0])) not in tc.links:
                stats["link-unconfirmed"] += 1
    return located, null_located, stats


def classify(tc: ThreeC, kind: str, func: str, ia: list[str], ea: list[str]) -> dict:
    il = [tc.checked(a) for a in ia]
    el = [tc.checked(a) for a in ea]
    if el and el[0] == "WILD":
        label = "wild"
    elif il == el:
        label = "checked"
    else:
        label = "itype"
    wild_atom = ""
    if label == "wild":
        wild_atom = ea[0]
    elif label == "itype" and il and il[0] == "WILD":
        wild_atom = ia[0]
    origin, reasons = "", []
    if wild_atom:
        cands = [wild_atom] + ([ia[0]] if ia[0] != wild_atom else [])
        for a in cands:
            for r in tc.direct.get(a, []):
                if r not in reasons:
                    reasons.append(r)
        if reasons:
            origin = "direct"
        else:
            origin = "indirect"
            for root in tc.roots_of.get(wild_atom, []):
                reasons.append(f"via {root}: " + "; ".join(tc.direct.get(root, [])))
    atoms = ",".join(ia) + ("" if ia == ea else " / " + ",".join(ea))
    return {
        "_ia": ia, "_ea": ea, "_label3": label, "_wild_atom": wild_atom,
        "Name": ia[0],
        "Reasons": " | ".join(reasons),
        "3CCheckedKind": "" if label == "wild" else (el[0] if el else ""),
        "3CWildOrigin": origin,
        "3CKind": kind,
        "3CInternal": "|".join(il),
        "3CExternal": "|".join(el),
        "3CAtoms": atoms,
        "3CFunction": func,
    }


def threec_values(rec: dict, label_mode: str, binary_itype: str,
                  rc_map: dict[str, dict]) -> dict[str, str]:
    lab = rec["_label3"]
    if label_mode == "binary":
        lab = "wild" if lab == "wild" or (lab == "itype" and binary_itype == "wild") \
            else "not wild"
    rc = rc_map.get(rec["_wild_atom"] or rec["Name"], {})
    loc = rec.get("loc")
    out = {
        "Name": rec["Name"],
        "Reasons": rec["Reasons"],
        "PtrsAffected": str(rc.get("PtrsAffected", "")),
        "PtrsScore": str(rc.get("PtrsScore", "")),
        "InSrc": str(rc.get("InSrc", "")),
        "SubReasons": json.dumps(rc["SubReasons"], ensure_ascii=False)
        if "SubReasons" in rc else "",
        "3CClassification": lab,
    }
    for k in ("3CCheckedKind", "3CWildOrigin", "3CKind", "3CInternal",
              "3CExternal", "3CAtoms", "3CFunction",
              "3CVoidPtr", "3CVoidEvidence", "3CGeneric"):
        out[k] = rec[k]
    out.update({
        "3CFile": loc[0] if loc else "", "3CLine": str(loc[1]) if loc else "",
        "3CBeginCol": str(loc[2]) if loc else "", "3CEndCol": str(loc[3]) if loc else "",
    })
    return out


def loc_sort_key(row: dict[str, str]) -> tuple:
    def num(v: str) -> tuple[int, int]:
        v = (v or "").strip()
        return (0, int(v)) if v.isdigit() else (1, 0)
    return (num(row["line"]), num(row["beginCol"]),
            num(row["endLine"]), num(row["endCol"]))


# ------------------------------------------------------------------------- main
def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("threec_json", type=Path, help="3C <prefix>_final_output.json")
    ap.add_argument("genc2rust_csv", type=Path, help="GenC2Rust taints CSV")
    ap.add_argument("--out", type=Path, help="write CSV here instead of stdout")
    ap.add_argument("--3c-label-mode", dest="label_mode", choices=["tri", "binary"],
                    default="tri", help="tri: checked|itype|wild; binary: wild|not wild")
    ap.add_argument("--binary-itype", choices=["not-wild", "wild"], default="not-wild",
                    help="binary mode: how an itype is labelled (paper counts itypes as converted)")
    ap.add_argument("--3c-root-causes", dest="root_causes", type=Path,
                    help="PerWildPtrStats.json from the same 3C run (fills PtrsAffected etc.)")
    ap.add_argument("--trace-indirect-roots", action="store_true",
                    help="name root causes of indirectly wild pointers in Reasons")
    ap.add_argument("--void-reason", default="Default void* type",
                    help="3C reason text that marks a void* (fallback void evidence)")
    ap.add_argument("--3c-file", dest="file_filter",
                    help="keep only 3C subjects whose file path-suffix-matches this")
    ap.add_argument("--3c-sentinel", dest="sentinel", default=DEFAULT_3C_SENTINEL,
                    help="3C column value on genc2rust-only rows")
    ap.add_argument("--no-return-match", action="store_true",
                    help="disable return matching (returns become genc2rust-only)")
    ap.add_argument("--return-nearby", type=int, default=2,
                    help="lines after a return's begin line searched for the function name (0=off)")
    ap.add_argument("--line-fallback", choices=["unique", "all", "off"], default="unique")
    ap.add_argument("--no-line-fallback", action="store_true",
                    help="alias for --line-fallback off")
    ap.add_argument("--drop-null-3c-location", action="store_true",
                    help="omit 3C subjects with no source location")
    ap.add_argument("--begin-line-col", default="begin line")
    ap.add_argument("--begin-col-col", default="begin col")
    ap.add_argument("--end-line-col", default="end line")
    ap.add_argument("--end-col-col", default="end col")
    args = ap.parse_args(argv)
    if args.no_line_fallback:
        args.line_fallback = "off"

    # --- load 3C ---
    try:
        data = json.loads(args.threec_json.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        raise SystemExit(f"error: cannot read 3C JSON {args.threec_json}: {exc}")
    if not isinstance(data, dict) or "ConstraintVariables" not in data:
        raise SystemExit("error: not a 3C final_output.json (no 'ConstraintVariables'); "
                         "run 3c with -dump-intermediate")
    tc = ThreeC(data, trace_roots=args.trace_indirect_roots, void_reason=args.void_reason)
    located, null_located, sstats = build_subjects(data, tc, args.file_filter)

    rc_map: dict[str, dict] = {}
    if args.root_causes:
        rc = json.loads(args.root_causes.read_text(encoding="utf-8"))
        for e in (rc.get("RootCauseStats", []) if isinstance(rc, dict) else rc):
            rc_map[str(e.get("Name", ""))] = e

    files = Counter(r["loc"][0] for r in located)
    if len(files) > 1 and not args.file_filter:
        print("warning: 3C subjects span several files; matching ignores the file "
              "(use --3c-file): " + ", ".join(f"{f} ({n})" for f, n in files.most_common()),
              file=sys.stderr)

    # indices
    exact_index: dict[tuple, list[int]] = {}
    line_index: dict[int, list[int]] = {}
    ret_index: dict[int, list[int]] = {}
    for i, rec in enumerate(located):
        _, ln, sc, _ = rec["loc"]
        if rec["3CKind"] == "return":
            ret_index.setdefault(ln, []).append(i)
        else:
            exact_index.setdefault((ln, sc), []).append(i)
            line_index.setdefault(ln, []).append(i)
    ret_lines = sorted(ret_index)

    # --- read GenC2Rust CSV ---
    with args.genc2rust_csv.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames is None:
            raise SystemExit("error: GenC2Rust CSV appears to be empty")
        gr_fields = list(reader.fieldnames)
        bl_c = resolve_col(gr_fields, args.begin_line_col)
        bc_c = resolve_col(gr_fields, args.begin_col_col)
        el_c = resolve_col(gr_fields, args.end_line_col)
        ec_c = resolve_col(gr_fields, args.end_col_col)
        gr_rows = list(reader)

    def gi(row: dict[str, str], col: str) -> int | None:
        try:
            return int((row.get(col) or "").strip())
        except ValueError:
            return None

    parsed = []
    gr_cols_on_line: dict[int, set[int]] = {}
    for row in gr_rows:
        bl, bc, el, ec = gi(row, bl_c), gi(row, bc_c), gi(row, el_c), gi(row, ec_c)
        is_ret = bl is not None and (bc == 1 or (el is not None and el != bl))
        parsed.append((row, bl, bc, el, ec, is_ret))
        if bl is not None and bc is not None and not is_ret:
            gr_cols_on_line.setdefault(bl, set()).add(bc)

    out_fields = ["match", "match_rule"] + UNIFIED_COLS + gr_fields + THREEC_COLS + THREEC_NEW_COLS
    gr_sentinel = {f: GENC_SENTINEL for f in gr_fields}
    c3_sentinel = {f: args.sentinel for f in THREEC_COLS + THREEC_NEW_COLS}

    matched: set[int] = set()
    out_rows: list[dict[str, str]] = []
    n = Counter()
    for row, bl, bc, el, ec, is_ret in parsed:
        pairs: list[tuple[int, str]] = []
        if bl is not None:
            if is_ret:
                if not args.no_return_match:
                    pairs = [(i, "return") for i in ret_index.get(bl, [])]
                    if not pairs and args.return_nearby > 0:
                        near = [ln for ln in ret_lines if bl < ln <= bl + args.return_nearby]
                        if near:
                            pairs = [(i, "return-nearby") for i in ret_index[near[0]]]
            else:
                exact = exact_index.get((bl, bc), []) if bc is not None else []
                if exact:
                    pairs = [(i, "exact") for i in exact]
                elif args.line_fallback != "off":
                    taken = gr_cols_on_line.get(bl, set())
                    cands = [i for i in line_index.get(bl, [])
                             if located[i]["loc"][2] not in taken]
                    if args.line_fallback == "all" or len(cands) == 1:
                        pairs = [(i, "line-only") for i in cands]
        base = {"line": "" if bl is None else str(bl),
                "beginCol": "" if bc is None else str(bc),
                "endLine": "" if el is None else str(el),
                "endCol": "" if ec is None else str(ec)}
        base.update({f: row.get(f, "") for f in gr_fields})
        if pairs:
            for idx, rule in pairs:
                matched.add(idx)
                out = {"match": "both", "match_rule": rule, **base}
                out.update(threec_values(located[idx], args.label_mode,
                                         args.binary_itype, rc_map))
                out_rows.append(out)
                n[rule] += 1
        else:
            out_rows.append({"match": "genc2rust-only", "match_rule": "", **base,
                             **c3_sentinel})
            n["gr_only"] += 1

    for i, rec in enumerate(located):
        if i in matched:
            continue
        _, ln, sc, ecol = rec["loc"]
        out = {"match": "3c-only", "match_rule": "", "line": str(ln),
               "beginCol": str(sc), "endLine": str(ln), "endCol": str(ecol),
               **gr_sentinel}
        out.update(threec_values(rec, args.label_mode, args.binary_itype, rc_map))
        out_rows.append(out)
        n["3c_only"] += 1
    if not args.drop_null_3c_location:
        for rec in null_located:
            out = {"match": "3c-only", "match_rule": "", "line": "", "beginCol": "",
                   "endLine": "", "endCol": "", **gr_sentinel}
            out.update(threec_values(rec, args.label_mode, args.binary_itype, rc_map))
            out_rows.append(out)
            n["3c_null"] += 1

    out_rows.sort(key=loc_sort_key)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
    out_fh = args.out.open("w", newline="", encoding="utf-8") if args.out else sys.stdout
    try:
        w = csv.DictWriter(out_fh, fieldnames=out_fields)
        w.writeheader()
        w.writerows(out_rows)
    finally:
        if args.out:
            out_fh.close()

    labels = Counter(r["_label3"] for r in located + null_located)
    both = sum(n[k] for k in ("exact", "return", "return-nearby", "line-only"))
    print(
        f"3C subjects: {len(located)} located + {len(null_located)} null-location "
        f"(var={sstats['var']}, param={sstats['param']}, return={sstats['return']}; "
        f"skipped non-pointer={sstats['non-pointer'] + sstats['non-pointer-return']}, "
        f"duplicates={sstats['duplicate']}, file-filtered={sstats['filtered-file']}, "
        f"int/ext link unconfirmed={sstats['link-unconfirmed']})\n"
        f"3C labels: " + ", ".join(f"{k}={v}" for k, v in sorted(labels.items())) + "\n"
        f"GenC2Rust rows: {len(gr_rows)}\n"
        f"output: {len(out_rows)} rows -> both={both} (exact={n['exact']}, "
        f"return={n['return']}, return-nearby={n['return-nearby']}, "
        f"line-only={n['line-only']}), genc2rust-only={n['gr_only']}, "
        f"3c-only={n['3c_only'] + n['3c_null']} (incl. {n['3c_null']} null-location)",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

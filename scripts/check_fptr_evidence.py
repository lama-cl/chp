#!/usr/bin/env python3
"""Pin and check function-pointer (fptr) extension evidence.

Every fptr fixture carries machine-readable comment markers that state the
fact it exercises, the subject(s) to inspect, what the analyzer reports today
(``@baseline``) and what the fptr extension must report (``@expected``)::

    // @fptr-fact F05 const-dispatch-table
    // @subject   run::ctx
    // @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback
    // @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1

Markers may also be written inside block comments (`` * @subject run::ctx``).
Several ``@subject`` blocks may appear in one fixture; ``@baseline`` and
``@expected`` attach to the most recent ``@subject``.  Markers may live in any
.c file of a multi-TU project; the subject is looked up across all TUs.

Fixtures discovered by default:
  examples/fptr-*.c                         single TU, run directly
  test_projects/projects/p*_fptr_*/         multi TU, run through
                                            scripts/run_all_multitu_projects.sh

Checked keys (all optional per marker line):
  category    paper-facts primaryCategory
  callback    paper-facts callbackMediation.state
  behavior    paper-facts behavior (WildPointerBehaviorClass)
  types       sorted refinementFacts.activeConcreteTypes + activeByteTypes,
              comma-joined, spaces as '_' ('const int *' -> const_int*); "-" for none
  resolution  indirectCallResolution "<state>@<layer>" (new fptr facet, see
              design/fptr-extension/02-design.md); "<absent>" until emitted
  sigmismatch indirectCallResolution.signatureMismatch ("true"/"false")
A missing subject is reported as category=<absent>.

Modes:
  --check baseline   exit 1 if any @baseline differs from current output.
                     Run before and after unrelated changes: the pinned
                     baseline is the regression evidence.
  --check expected   exit 1 if any @expected is not met (use once a milestone
                     claims a fixture; see 04-implementation-plan.md).
  --check none       report only (default).

Typical use from the repository root (tool built in ./build):
  python3 scripts/check_fptr_evidence.py --check baseline
  python3 scripts/check_fptr_evidence.py --markdown design/fptr-extension/evidence/evidence-table.md
  python3 scripts/check_fptr_evidence.py --only F05 --show-observed
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
KEYS = ("category", "callback", "behavior", "types", "resolution", "sigmismatch")
MARKER = re.compile(r"^\s*(?://|\*)\s*@(fptr-fact|subject|baseline|expected)\b\s*(.*?)\s*$")


@dataclass
class Subject:
    name: str
    baseline: dict = field(default_factory=dict)
    expected: dict = field(default_factory=dict)


@dataclass
class Fixture:
    path: Path
    multi_tu: bool
    fact_id: str = ""
    fact_name: str = ""
    subjects: list = field(default_factory=list)
    observed: dict = field(default_factory=dict)  # subject -> dict
    error: str = ""


def parse_kv(text: str, where: str) -> dict:
    out = {}
    for tok in text.split():
        if "=" not in tok:
            raise SystemExit(f"{where}: malformed marker token '{tok}'")
        k, v = tok.split("=", 1)
        if k not in KEYS:
            raise SystemExit(f"{where}: unknown key '{k}' (allowed: {', '.join(KEYS)})")
        out[k] = v
    return out


def parse_markers(fx: Fixture, files: list) -> None:
    current = None
    for f in files:
        for lineno, line in enumerate(f.read_text().splitlines(), 1):
            m = MARKER.match(line)
            if not m:
                continue
            tag, rest = m.groups()
            where = f"{f}:{lineno}"
            if tag == "fptr-fact":
                parts = rest.split(None, 1)
                fx.fact_id = parts[0] if parts else ""
                fx.fact_name = parts[1] if len(parts) > 1 else ""
            elif tag == "subject":
                current = Subject(rest)
                fx.subjects.append(current)
            else:
                if current is None:
                    raise SystemExit(f"{where}: @{tag} before any @subject")
                getattr(current, tag).update(parse_kv(rest, where))
    if not fx.fact_id:
        raise SystemExit(f"{fx.path}: missing @fptr-fact marker")
    if not fx.subjects:
        raise SystemExit(f"{fx.path}: no @subject marker")


def normalize_type(t: str) -> str:
    """'const int *' -> 'const_int*' (marker values are whitespace-separated)."""
    t = re.sub(r"\s*\*", "*", t.strip())
    return re.sub(r"\s+", "_", t)


def subject_values(s: dict) -> dict:
    rf = s.get("refinementFacts", {})
    types = sorted(set(rf.get("activeConcreteTypes", [])) | set(rf.get("activeByteTypes", [])))
    icr = s.get("indirectCallResolution")
    if isinstance(icr, dict):
        resolution = f"{icr.get('state', '?')}@{icr.get('layer', '?')}"
        sigmismatch = "true" if icr.get("signatureMismatch") else "false"
    else:
        resolution, sigmismatch = "<absent>", "<absent>"
    return {
        "category": s.get("primaryCategory", "<absent>"),
        "callback": s.get("callbackMediation", {}).get("state", "<absent>"),
        "behavior": s.get("behavior", "<absent>"),
        "types": ",".join(normalize_type(t) for t in types) or "-",
        "resolution": resolution,
        "sigmismatch": sigmismatch,
    }


def collect(fx: Fixture, fact_files: list) -> None:
    by_entity = {}
    for jf in fact_files:
        data = json.loads(Path(jf).read_text())
        for s in data.get("subjects", []):
            by_entity.setdefault(s.get("entity"), s)
    for sub in fx.subjects:
        s = by_entity.get(sub.name)
        fx.observed[sub.name] = subject_values(s) if s else {k: "<absent>" for k in KEYS}


def run_single(fx: Fixture, tool: Path, work: Path) -> None:
    out = work / (fx.path.stem + ".json")
    cmd = [str(tool), str(fx.path), f"--paper-facts-json={out}", "--", "-std=c17"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or not out.exists():
        fx.error = f"tool failed ({r.returncode}): {r.stderr.strip()[-300:]}"
        return
    collect(fx, [out])


def run_projects(fixtures: list, tool: Path, work: Path, rounds: int) -> None:
    if not fixtures:
        return
    proj_root = work / "projects"
    proj_root.mkdir()
    for fx in fixtures:
        dst = proj_root / fx.path.name
        shutil.copytree(fx.path, dst, ignore=shutil.ignore_patterns("expected", "*.json"))
        entries = [
            {"directory": str(dst), "file": str(c),
             "arguments": ["clang", "-std=c17", "-c", str(c), "-o", str(c.with_suffix(".o"))]}
            for c in sorted(dst.glob("*.c"))
        ]
        (dst / "compile_commands.json").write_text(json.dumps(entries, indent=2))
    out_root = work / "multitu-out"
    env = dict(os.environ, TOOL=str(tool), TOOL_ROOT=str(REPO),
               PROJECT_GLOB=str(proj_root), OUTPUT_ROOT=str(out_root),
               PAPER_FACTS="1", SUMMARY_IMPORT_ROUNDS=str(rounds))
    r = subprocess.run(["bash", str(REPO / "scripts/run_all_multitu_projects.sh")],
                       cwd=str(tool.parent), env=env, capture_output=True, text=True)
    (work / "multitu.log").write_text(r.stdout + r.stderr)
    for fx in fixtures:
        facts = sorted((out_root / fx.path.name / "paper-facts").glob("*.json"))
        if not facts:
            fx.error = f"no paper facts produced (see {work / 'multitu.log'})"
            continue
        collect(fx, facts)


def compare(want: dict, got: dict) -> list:
    return [f"{k}: want {v} got {got.get(k)}" for k, v in want.items() if got.get(k) != v]


def discover(only: list) -> list:
    fixtures = [Fixture(p, False) for p in sorted((REPO / "examples").glob("fptr-*.c"))]
    fixtures += [Fixture(p, True) for p in sorted((REPO / "test_projects/projects").glob("p*_fptr_*"))
                 if p.is_dir()]
    for fx in fixtures:
        parse_markers(fx, [fx.path] if not fx.multi_tu else sorted(fx.path.glob("*.c")))
    if only:
        fixtures = [f for f in fixtures if f.fact_id in only or f.path.name in only
                    or f.path.stem in only]
    return fixtures


def rel(p: Path) -> str:
    try:
        return str(p.relative_to(REPO))
    except ValueError:
        return str(p)


def markdown(fixtures: list) -> str:
    lines = [
        "# fptr evidence table",
        "",
        "Generated by `python3 scripts/check_fptr_evidence.py --markdown <this file>`.",
        "Do not edit by hand. `baseline` = current analyzer output (pinned);",
        "`expected` = state required once the fptr extension milestone lands.",
        "",
        "| Fact | Fixture | Subject | Baseline (today) | Expected | Baseline pinned | Expected met |",
        "|---|---|---|---|---|---|---|",
    ]
    for fx in fixtures:
        for sub in fx.subjects:
            got = fx.observed.get(sub.name, {})
            b = "error" if fx.error else ("yes" if not compare(sub.baseline, got) else "**NO**")
            e = "error" if fx.error else ("yes" if not compare(sub.expected, got) else "xfail")
            fmt = lambda d: "<br>".join(f"{k}={v}" for k, v in d.items())
            lines.append(f"| {fx.fact_id} | `{rel(fx.path)}` | `{sub.name}` | {fmt(sub.baseline)} "
                         f"| {fmt(sub.expected)} | {b} | {e} |")
    return "\n".join(lines) + "\n"


def write_project_readmes(fixtures: list) -> None:
    """Regenerate expected/README.expected.txt of multi-TU fixtures from the
    leading block comment of each .c file (markers stay the source of truth)."""
    for fx in fixtures:
        if not fx.multi_tu:
            continue
        prose = []
        for f in sorted(fx.path.glob("*.c")):
            m = re.match(r"\s*/\*(.*?)\*/", f.read_text(), re.S)
            if not m:
                continue
            for ln in m.group(1).splitlines():
                ln = re.sub(r"^\s*\*\s?", "", ln).rstrip()
                if ln.strip() and not MARKER.match(" * " + ln):
                    prose.append(ln)
        out = [f"Expected fptr-extension behavior for {fx.path.name}.", "",
               f"Fact: {fx.fact_id} {fx.fact_name}", ""] + prose + [""]
        for sub in fx.subjects:
            kv = lambda d: " ".join(f"{k}={v}" for k, v in d.items()) or "(none)"
            out += [f"Subject {sub.name}:",
                    f"  today (pinned baseline):  {kv(sub.baseline)}",
                    f"  required after extension: {kv(sub.expected)}", ""]
        out += ["Machine check (the @-markers in the .c files are the source of truth;",
                "regenerate this file with --write-project-readmes):",
                f"  python3 scripts/check_fptr_evidence.py --only {fx.path.name}",
                "Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md", ""]
        (fx.path / "expected").mkdir(exist_ok=True)
        (fx.path / "expected" / "README.expected.txt").write_text("\n".join(out))


def pin_baselines(fixtures: list) -> None:
    """Rewrite every @baseline marker of the given fixtures with observed values.

    Only for fixtures a milestone deliberately changes (see
    04-implementation-plan.md); re-pinning everything hides regressions."""
    for fx in fixtures:
        if fx.error:
            continue
        files = [fx.path] if not fx.multi_tu else sorted(fx.path.glob("*.c"))
        for f in files:
            out = []
            for ln in f.read_text().splitlines(keepends=True):
                m = MARKER.match(ln)
                if m and m.group(1) == "baseline":
                    continue
                out.append(ln)
                if m and m.group(1) == "subject":
                    got = fx.observed[m.group(2)]
                    keys = ["category", "callback", "behavior", "types"] + [
                        k for k in ("resolution", "sigmismatch") if got[k] != "<absent>"]
                    prefix = " * " if ln.lstrip().startswith("*") else "// "
                    out.append(f"{prefix}@baseline  " + " ".join(f"{k}={got[k]}" for k in keys) + "\n")
            f.write_text("".join(out))
        print(f"pinned {fx.fact_id} {rel(fx.path)}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tool", default=str(REPO / "build/cast-history-tool"))
    ap.add_argument("--check", choices=("none", "baseline", "expected"), default="none")
    ap.add_argument("--only", nargs="*", default=[], help="fact ids (F05) or fixture names")
    ap.add_argument("--rounds", type=int, default=2, help="multi-TU SUMMARY_IMPORT_ROUNDS")
    ap.add_argument("--markdown", help="write evidence table to this path")
    ap.add_argument("--show-observed", action="store_true",
                    help="print observed values in marker syntax (for authoring baselines)")
    ap.add_argument("--keep-work", action="store_true")
    ap.add_argument("--pin-baselines", action="store_true",
                    help="rewrite @baseline markers of the selected fixtures from observed output "
                         "(requires --only; use for fixtures a milestone claims)")
    ap.add_argument("--write-project-readmes", action="store_true",
                    help="regenerate expected/README.expected.txt of multi-TU fixtures and exit")
    args = ap.parse_args()
    if args.write_project_readmes:
        write_project_readmes(discover(args.only))
        return 0

    tool = Path(args.tool).resolve()
    if not os.access(tool, os.X_OK):
        print(f"error: tool not executable: {tool}", file=sys.stderr)
        return 2
    fixtures = discover(args.only)
    work = Path(tempfile.mkdtemp(prefix="fptr-evidence-"))
    try:
        for fx in fixtures:
            if not fx.multi_tu:
                run_single(fx, tool, work)
        run_projects([f for f in fixtures if f.multi_tu], tool, work, args.rounds)

        bad_base = bad_exp = errors = 0
        for fx in fixtures:
            if fx.error:
                errors += 1
                print(f"ERROR  {fx.fact_id:4s} {rel(fx.path)}: {fx.error}")
                continue
            for sub in fx.subjects:
                got = fx.observed[sub.name]
                db, de = compare(sub.baseline, got), compare(sub.expected, got)
                bad_base += bool(db)
                bad_exp += bool(de)
                status = ("BASELINE-DRIFT" if db else "baseline-ok") + " / " + \
                         ("expected-met" if not de else "xfail")
                print(f"{status:30s} {fx.fact_id:4s} {rel(fx.path)} :: {sub.name}")
                for d in db:
                    print(f"    baseline  {d}")
                if args.show_observed:
                    print("    observed  " + " ".join(
                        f"{k}={got[k]}" for k in ("category", "callback", "behavior", "types")))
        if args.pin_baselines:
            if not args.only:
                print("error: --pin-baselines requires --only", file=sys.stderr)
                return 2
            pin_baselines(fixtures)
            write_project_readmes(fixtures)
        n = sum(len(f.subjects) for f in fixtures)
        print(f"\n{len(fixtures)} fixtures, {n} subjects: baseline drift={bad_base}, "
              f"expected unmet={bad_exp}, errors={errors}")
        if args.markdown:
            Path(args.markdown).parent.mkdir(parents=True, exist_ok=True)
            Path(args.markdown).write_text(markdown(fixtures))
        if errors:
            return 1
        if args.check == "baseline" and bad_base:
            return 1
        if args.check == "expected" and bad_exp:
            return 1
        return 0
    finally:
        if args.keep_work:
            print(f"work dir kept: {work}")
        else:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())

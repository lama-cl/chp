#!/usr/bin/env python3
"""Run the pointer-coverage gates against a built cast-history-tool.

Each gate in gates.json names a fixture (one or more C files) and a list of
checks over the tool's paper-facts / report output.  Checks are either

  control  already true on the unmodified tool; must never turn false
  target   the behaviour the gate's phase has to deliver

A gate is GREEN when all of its checks pass.  The runner ratchets against
baseline.json: any check that passed there and fails now is a REGRESSION.

Typical use:
  run_gates.py --tool build/cast-history-tool                 # status + ratchet
  run_gates.py --tool ... --gate P1-fp-array-dispatch -v      # work on one gate
  run_gates.py --tool ... --require-phase P1                  # phase exit gate
  run_gates.py --tool ... --update-baseline                   # lock in progress

Exit status: 0 ok, 1 regression or a required gate is not green, 2 the
pipeline itself failed (tool missing, crash, unreadable output).
"""
import argparse
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
import coverage_ledger  # noqa: E402


def norm_type(text):
    return " ".join(text.replace("*", " * ").split())


def dig(obj, dotted):
    for part in dotted.split("."):
        if not isinstance(obj, dict) or part not in obj:
            return None
        obj = obj[part]
    return obj


class PipelineError(Exception):
    pass


def run(cmd, log):
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    log.write_text(proc.stdout)
    return proc.returncode


def analyse(tool, sources, workdir, std):
    """C1 -> summary index -> one import round.  Returns (subjects, features)."""
    root = str(HERE / "fixtures")
    flags = ["--", f"-std={std}"]

    def stage(name, extra):
        out = workdir / name
        for sub in ("summaries", "paper-facts", "reports"):
            (out / sub).mkdir(parents=True, exist_ok=True)
        cmd = [tool, *extra,
               "--summary-dir", str(out / "summaries"),
               "--paper-facts-dir", str(out / "paper-facts"),
               "--paper-facts-project-root", root,
               "--summary-report-dir", str(out / "reports"),
               *sources, *flags]
        rc = run(cmd, out / "tool.log")
        if rc != 0:
            raise PipelineError(f"{name}: tool exited {rc}; see {out / 'tool.log'}")
        return out

    c1 = stage("c1", [])
    index = workdir / "project-summary-index.json"
    rc = run([sys.executable, str(REPO / "scripts" / "build_summary_index.py"),
              str(c1 / "summaries"), "-o", str(index)], workdir / "index.log")
    if rc != 0 or not index.exists():
        raise PipelineError(f"build_summary_index.py exited {rc}; see {workdir / 'index.log'}")
    final = stage("import", ["--enable-summary-import", "--summary-index", str(index)])

    subjects, features = [], []
    for path in sorted((final / "paper-facts").glob("*.json")):
        subjects.extend(json.loads(path.read_text()).get("subjects") or [])
    for path in sorted((final / "reports").glob("*.json")):
        features.extend(json.loads(path.read_text()).get("featureDiagnostics") or [])
    if not subjects:
        raise PipelineError(f"no paper-facts subjects produced under {final}")
    return subjects, features


def check_subject_row(check, row):
    """Return a list of failure strings for one candidate subject row."""
    fails = []
    types = coverage_ledger.concrete_types(row)
    want = {norm_type(t) for t in check.get("typesInclude", [])}
    if want - types:
        fails.append(f"missing types {sorted(want - types)} (has {sorted(types)})")
    banned = {norm_type(t) for t in check.get("typesExclude", [])}
    if banned & types:
        fails.append(f"forbidden types present {sorted(banned & types)}")
    bucket = coverage_ledger.bucket_of(row)
    if "bucket" in check and bucket != check["bucket"]:
        fails.append(f"bucket {bucket!r} != {check['bucket']!r}")
    if "bucketPrefix" in check and not bucket.startswith(check["bucketPrefix"]):
        fails.append(f"bucket {bucket!r} lacks prefix {check['bucketPrefix']!r}")
    state = (row.get("callbackMediation") or {}).get("state")
    if state in check.get("callbackStateNotIn", []):
        fails.append(f"callbackMediation.state is still {state!r}")
    for dotted, expected in (check.get("fields") or {}).items():
        actual = dig(row, dotted)
        if isinstance(expected, list):
            ok = isinstance(actual, list) and all(e in actual for e in expected)
        else:
            ok = actual == expected
        if not ok:
            fails.append(f"{dotted} = {actual!r}, expected {expected!r}")
    return fails


def evaluate(check, subjects, features):
    if check["kind"] == "noFeature":
        hits = [f for f in features
                if f.get("severity") == "unsupported" and f.get("feature") in check["features"]]
        if not hits:
            return []
        where = sorted({f"{h['feature']}@{pathlib.Path(h['location']['file']).name}:"
                        f"{h['location']['line']}" for h in hits})
        return [f"unsupported feature still reported: {', '.join(where)}"]
    if check["kind"] == "subject":
        rows = [s for s in subjects if s.get("entity") == check["entity"]]
        if not rows:
            return [f"no subject row for entity {check['entity']!r}"]
        results = [check_subject_row(check, r) for r in rows]
        if "typesExclude" in check:  # exclusions must hold on every row
            merged = [f for r in results for f in r if f.startswith("forbidden")]
            if merged:
                return merged
        return min(results, key=len)
    return [f"unknown check kind {check['kind']!r}"]


def describe(check):
    if check["kind"] == "noFeature":
        return "no unsupported " + "/".join(check["features"])
    bits = [check["entity"]]
    if "typesInclude" in check:
        bits.append("has " + ", ".join(check["typesInclude"]))
    if "typesExclude" in check:
        bits.append("lacks " + ", ".join(check["typesExclude"]))
    for key in ("bucket", "bucketPrefix"):
        if key in check:
            bits.append(f"{key}={check[key]}")
    if "callbackStateNotIn" in check:
        bits.append("callback resolved")
    for dotted, value in (check.get("fields") or {}).items():
        bits.append(f"{dotted}={json.dumps(value)}")
    return "; ".join(bits)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tool", default=os.environ.get("CAST_HISTORY_TOOL"),
                    help="path to cast-history-tool (or $CAST_HISTORY_TOOL)")
    ap.add_argument("--gate", action="append", default=[], help="run only this gate id (repeatable)")
    ap.add_argument("--phase", action="append", default=[], help="run only gates of this phase (repeatable)")
    ap.add_argument("--require", action="append", default=[], help="gate id that must be green (repeatable)")
    ap.add_argument("--require-phase", action="append", default=[],
                    help="every gate of this phase must be green (repeatable)")
    ap.add_argument("--update-baseline", action="store_true", help="rewrite baseline.json from this run")
    ap.add_argument("--baseline", default=str(HERE / "baseline.json"))
    ap.add_argument("--json", help="write machine-readable results here")
    ap.add_argument("--keep", help="keep tool outputs in this directory")
    ap.add_argument("--std", default="c17")
    ap.add_argument("--list", action="store_true", help="list gates and exit")
    ap.add_argument("-v", "--verbose", action="store_true", help="show passing checks too")
    args = ap.parse_args()

    manifest = json.loads((HERE / "gates.json").read_text())
    gates = manifest["gates"]
    known = {g["id"] for g in gates}
    for gid in args.gate + args.require:
        if gid not in known:
            ap.error(f"unknown gate id {gid!r}")
    if args.list:
        for g in gates:
            print(f"{g['id']:36s} {g['phase']}  {g['title']}")
        return 0
    if not args.tool or not os.access(args.tool, os.X_OK):
        print("error: pass --tool /path/to/cast-history-tool (or set $CAST_HISTORY_TOOL)", file=sys.stderr)
        return 2
    selected = [g for g in gates
                if (not args.gate or g["id"] in args.gate) and (not args.phase or g["phase"] in args.phase)]
    if args.update_baseline and len(selected) != len(gates):
        ap.error("--update-baseline needs a full run (no --gate/--phase filter)")

    baseline_path = pathlib.Path(args.baseline)
    baseline = json.loads(baseline_path.read_text())["checks"] if baseline_path.exists() else {}
    workroot = pathlib.Path(args.keep) if args.keep else pathlib.Path(tempfile.mkdtemp(prefix="ptrcov-gates-"))
    workroot.mkdir(parents=True, exist_ok=True)

    results, cache = {}, {}
    regressions, newly_green, errors = [], [], []
    gate_status = {}
    for gate in selected:
        key = tuple(gate["fixture"])
        if key not in cache:
            workdir = workroot / gate["id"]
            shutil.rmtree(workdir, ignore_errors=True)
            workdir.mkdir(parents=True)
            try:
                cache[key] = analyse(args.tool, [str(HERE / f) for f in key], workdir, args.std)
            except (PipelineError, OSError, ValueError) as err:
                cache[key] = err
        outcome = cache[key]
        lines, green = [], True
        for i, check in enumerate(gate["checks"]):
            cid = f"{gate['id']}#{i}"
            if isinstance(outcome, Exception):
                fails = [f"pipeline error: {outcome}"]
            else:
                fails = evaluate(check, *outcome)
            status = "fail" if fails else "pass"
            results[cid] = status
            green &= not fails
            was = baseline.get(cid)
            if fails and was == "pass":
                regressions.append(cid)
            if not fails and was == "fail":
                newly_green.append(cid)
            mark = "ok  " if not fails else ("REGR" if was == "pass" else "FAIL")
            if fails or args.verbose:
                lines.append(f"    [{mark}] {check['role']:7s} {describe(check)}")
                lines.extend(f"             -> {f}" for f in fails)
        if isinstance(outcome, Exception):
            errors.append(gate["id"])
        gate_status[gate["id"]] = "GREEN" if green else "RED"
        print(f"{gate_status[gate['id']]:5s} {gate['id']:36s} {gate['title']}")
        print("\n".join(lines) if lines else "", end="\n" if lines else "")

    required = set(args.require) | {g["id"] for g in gates if g["phase"] in args.require_phase}
    unmet = sorted(g for g in required if gate_status.get(g) != "GREEN")
    greens = sum(1 for s in gate_status.values() if s == "GREEN")
    print(f"\n{greens}/{len(gate_status)} gates green; "
          f"{len(regressions)} regression(s); {len(newly_green)} check(s) newly passing")
    if regressions:
        print("REGRESSIONS (passed in baseline.json, fail now): " + ", ".join(regressions))
    if unmet:
        print("REQUIRED GATES NOT GREEN: " + ", ".join(unmet))
    if newly_green and not args.update_baseline:
        print("newly passing checks are not locked in until you run --update-baseline")
    if not args.keep:
        shutil.rmtree(workroot, ignore_errors=True)
    else:
        print(f"tool outputs kept under {workroot}")

    if args.json:
        pathlib.Path(args.json).write_text(json.dumps(
            {"gates": gate_status, "checks": results, "regressions": regressions,
             "newlyPassing": newly_green, "requiredNotGreen": unmet}, indent=2) + "\n")
    if errors:
        print("PIPELINE ERRORS in: " + ", ".join(errors))
        return 2
    if args.update_baseline:
        if regressions:
            print("refusing to update baseline while regressions exist")
            return 1
        baseline_path.write_text(json.dumps(
            {"schemaName": "cast-history-pointer-coverage-baseline", "schemaVersion": 1,
             "checks": dict(sorted(results.items()))}, indent=2) + "\n")
        print(f"baseline written: {baseline_path}")
    return 1 if regressions or unmet else 0


if __name__ == "__main__":
    sys.exit(main())

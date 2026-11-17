#!/usr/bin/env python3
"""Run the cast-history pipeline over the pointer-coverage corpus and write
or check one coverage ledger per project.

Pipeline per project (same stages as scripts/run_all_multitu_projects.sh,
without needing a compile database): C1 per TU -> build_summary_index.py ->
one import round with paper facts -> coverage_ledger.

  run_corpus.py --tool build/cast-history-tool            # check against ledgers/
  run_corpus.py --tool ... --update                       # rewrite ledgers/
  run_corpus.py --tool ... --project vsftpd-3.0.3 --keep /tmp/out

Exit status: 0 ok, 1 a ledger regressed (or is missing), 2 pipeline failure.
"""
import argparse
import concurrent.futures
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


def run_tu(tool, extra, source, out, flags, root, timeout):
    cmd = [tool, *extra,
           "--summary-dir", str(out / "summaries"),
           "--paper-facts-dir", str(out / "paper-facts"),
           "--paper-facts-project-root", str(root),
           str(source), "--", *flags]
    try:
        proc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                              text=True, timeout=timeout, cwd=root)
        return source, proc.returncode, proc.stderr[-2000:]
    except subprocess.TimeoutExpired:
        return source, "timeout", ""


def stage(tool, extra, sources, out, flags, root, jobs, timeout):
    for sub in ("summaries", "paper-facts"):
        (out / sub).mkdir(parents=True, exist_ok=True)
    failed = []
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        futures = [pool.submit(run_tu, tool, extra, s, out, flags, root, timeout) for s in sources]
        for fut in futures:
            source, rc, err = fut.result()
            if rc != 0:
                failed.append(source.name)
                (out / (source.name + ".err")).write_text(f"rc={rc}\n{err}")
    return sorted(failed)


def analyse(tool, project, workdir, jobs, timeout, index_timeout=1800):
    root = (REPO / project["root"]).resolve()
    sources = sorted(p for d in project["dirs"] for p in (root / d).glob("*.c"))
    if not sources:
        raise RuntimeError(f"no sources under {root}")
    c1_failed = stage(tool, [], sources, workdir / "c1", project["flags"], root, jobs, timeout)
    index = workdir / "project-summary-index.json"
    try:
        proc = subprocess.run([sys.executable, str(REPO / "scripts" / "build_summary_index.py"),
                               str(workdir / "c1" / "summaries"), "-o", str(index)],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                              timeout=index_timeout)
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"build_summary_index.py exceeded {index_timeout}s")
    (workdir / "index.log").write_text(proc.stdout)
    if proc.returncode != 0 or not index.exists():
        raise RuntimeError(f"build_summary_index.py exited {proc.returncode}; see {workdir / 'index.log'}")
    ok = [s for s in sources if s.name not in c1_failed]
    failed = c1_failed + stage(tool, ["--enable-summary-import", "--summary-index", str(index)],
                               ok, workdir / "import", project["flags"], root, jobs, timeout)
    ledger = coverage_ledger.build_ledger(workdir / "import" / "paper-facts")
    ledger["project"] = project["name"]
    ledger["translationUnits"] = {"total": len(sources), "failed": len(failed), "failedNames": sorted(failed)}
    # Absolute paths differ between checkouts; keep examples repo-relative.
    ledger["blockedWithoutEscapeReasonExamples"] = [
        e.replace(str(REPO) + "/", "") for e in ledger["blockedWithoutEscapeReasonExamples"]]
    return ledger


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tool", default=os.environ.get("CAST_HISTORY_TOOL"))
    ap.add_argument("--project", action="append", default=[])
    ap.add_argument("--update", action="store_true", help="write ledgers/<project>.json")
    ap.add_argument("--ledger-dir", default=str(HERE / "ledgers"))
    ap.add_argument("--keep", help="keep tool outputs here")
    ap.add_argument("--include-slow", action="store_true", help="also run projects marked slow in corpus.json")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--timeout", type=int, default=600, help="seconds per TU")
    ap.add_argument("--index-timeout", type=int, default=1800, help="seconds per project index build")
    args = ap.parse_args()
    if not args.tool or not os.access(args.tool, os.X_OK):
        print("error: pass --tool /path/to/cast-history-tool", file=sys.stderr)
        return 2
    tool = str(pathlib.Path(args.tool).resolve())
    projects = json.loads((HERE / "corpus.json").read_text())["projects"]
    if not args.include_slow and not args.project:
        skipped = [p["name"] for p in projects if p.get("slow")]
        projects = [p for p in projects if not p.get("slow")]
        if skipped:
            print("skipping slow project(s) (use --include-slow): " + ", ".join(skipped))
    if args.project:
        unknown = set(args.project) - {p["name"] for p in projects}
        if unknown:
            ap.error(f"unknown project(s): {sorted(unknown)}")
        projects = [p for p in projects if p["name"] in args.project]
    ledger_dir = pathlib.Path(args.ledger_dir)
    workroot = pathlib.Path(args.keep) if args.keep else pathlib.Path(tempfile.mkdtemp(prefix="ptrcov-corpus-"))
    status = 0
    for project in projects:
        workdir = workroot / project["name"]
        shutil.rmtree(workdir, ignore_errors=True)
        workdir.mkdir(parents=True)
        try:
            ledger = analyse(tool, project, workdir, args.jobs, args.timeout, args.index_timeout)
        except (RuntimeError, OSError) as err:
            print(f"ERROR {project['name']}: {err}")
            status = 2
            continue
        tus = ledger["translationUnits"]
        void = ledger["kinds"]["void-data-pointer"]
        sites = ledger["indirectCallSites"]
        print(f"{project['name']:16s} TUs {tus['total'] - tus['failed']}/{tus['total']}  "
              f"void* resolved {void['resolved']}/{void['total']} blocked {void['blocked']}  "
              f"indirect sites unresolved {sites['unresolved']}/{sites['total']}")
        path = ledger_dir / f"{project['name']}.json"
        if args.update:
            ledger_dir.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(ledger, indent=2) + "\n")
            continue
        if not path.exists():
            print(f"  MISSING ledger {path}; run with --update")
            status = max(status, 1)
            continue
        old = json.loads(path.read_text())
        problems = coverage_ledger.compare(ledger, old)
        if tus["failed"] > old["translationUnits"]["failed"]:
            problems.append(f"failed TUs grew {old['translationUnits']['failed']} -> {tus['failed']}")
        for problem in problems:
            print("  REGRESSION:", problem)
        if problems:
            status = max(status, 1)
    if not args.keep:
        shutil.rmtree(workroot, ignore_errors=True)
    return status


if __name__ == "__main__":
    sys.exit(main())

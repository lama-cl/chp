#!/usr/bin/env python3
"""Regression driver for cast-history-tool.

Runs the regression set (see REGRESSION_EXAMPLES / REGRESSION_PROJECTS),
normalizes the output and compares it with expected files checked in next
to each fixture.  Design and rationale: design/regression-testing.md.

Commands:
  regress.py run    [--only NAME ...]   diff against expected files; exit 1 on mismatch
  regress.py update [--only NAME ...]   rewrite expected files from the current tool
  regress.py perf   [--only NAME ...]   compare timings with runtime baselines (warn-only)
  regress.py perf --update              rewrite runtime baselines (median of --runs)

NAME is a fixture name (demo1, p01_direct_cross_tu), or "examples" / "projects".

Expected-file layout:
  examples/expected/<demo>.expected.txt              --debug-dump output
  examples/expected/<demo>.runtime.expected.json     perf baseline
  <project>/expected/<output path>.expected.<ext>    every compared output file,
                                                     mirroring the driver output
  <project>/expected/runtime.expected.json           perf baseline
  <dir>/expected/meta.info                           LLVM version the files came from (not compared)

Exit status: 0 ok, 1 mismatch / fixture failure / strict perf regression,
2 usage or environment problem.
"""
from __future__ import annotations

import argparse
import difflib
import json
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

REPO = Path(__file__).resolve().parents[1]
EXAMPLES_DIR = REPO / "examples"
PROJECTS_DIR = REPO / "test_projects" / "projects"
EXAMPLES_DRIVER = REPO / "scripts" / "run_all_examples_normalized.sh"
PROJECT_DRIVER = REPO / "scripts" / "cast_history_project.sh"
COMPDB_SCRIPT = REPO / "scripts" / "regenerate_compile_commands.py"

MIN_LLVM_MAJOR = 19

# --------------------------------------------------------------- regression set
# Keep these lists short: the regression set is meant to run in well under a
# minute.  The full suite is still available through the drivers directly.
REGRESSION_EXAMPLES = ["demo1", "demo2", "demo3", "demo4", "demo5", "demo6"]
REGRESSION_PROJECTS = [
    "p01_direct_cross_tu",                                # acyclic direct cross-TU call
    "p02_cross_tu_return",                                # cross-TU return flow
    "p03_cross_tu_mutual_recursion",                      # cross-TU SCC
    "p91_deref_fnptr_higher_order_callee_normalization",  # fn pointers; verify_p91*.py
    "p126_recursive_parallel_observation_dedup",          # tests/verify_c6g5*_ checkers
]

# ---------------------------------------------------------------- normalization
# JSON fields removed before comparing, per output file name.  Each entry is a
# key path.  Keep this list explicit: every entry hides a real difference.
DROP_JSON_FIELDS: dict[str, list[tuple[str, ...]]] = {
    "project-summary-index.json": [
        # sha256 over *absolute* summary paths + bytes (build_summary_index.py)
        ("buildMetadata", "inputSummaryFingerprint"),
        # wall-clock timings; tracked by the perf baseline instead
        ("buildMetadata", "timingsMilliseconds"),
    ],
}

# Project output files that are compared (relative to the project output dir).
def is_compared_project_file(rel: Path) -> bool:
    if rel.parts and rel.parts[0] == "runtime-metrics":
        return False
    name = rel.name
    return (name == "project-summary-index.json"
            or name.endswith(".cast-summary.json")
            or name.endswith(".cast-prevalence.json")
            or rel.suffix == ".txt")


EXPECTED_MARK = ".expected"
META_NAME = "meta.info"   # not compared; records the LLVM version
RUNTIME_NAME = "runtime.expected.json"


def expected_name(rel: Path) -> Path:
    """a.c.txt -> a.c.expected.txt; summaries/a.c.cast-summary.json ->
    summaries/a.c.cast-summary.expected.json."""
    return rel.with_name(rel.stem + EXPECTED_MARK + rel.suffix)


def is_managed_expected(path: Path) -> bool:
    """Expected files owned by this script (everything else in expected/ is
    left alone, e.g. README.expected.txt or verify_*.py)."""
    name = path.name
    if name in ("README.expected.txt", RUNTIME_NAME):
        return False
    return bool(re.search(r"\.expected\.(txt|json)$", name))


def die(msg: str) -> None:
    """Usage or environment problem: exit status 2."""
    print(msg, file=sys.stderr)
    sys.exit(2)


def replace_paths(text: str, roots: list[tuple[Path, str]]) -> str:
    # Longest path first so nested roots (output dir inside the repo) win.
    for path, token in sorted(roots, key=lambda r: -len(str(r[0]))):
        text = text.replace(str(path), token)
    return text


def drop_field(obj: object, keys: tuple[str, ...]) -> None:
    for key in keys[:-1]:
        if not isinstance(obj, dict) or key not in obj:
            return
        obj = obj[key]
    if isinstance(obj, dict):
        obj.pop(keys[-1], None)


def normalize(path: Path, roots: list[tuple[Path, str]]) -> str:
    text = replace_paths(path.read_text(errors="replace"), roots)
    if path.suffix != ".json":
        return text if text.endswith("\n") else text + "\n"
    data = json.loads(text)
    for keys in DROP_JSON_FIELDS.get(path.name, []):
        drop_field(data, keys)
    return json.dumps(data, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


# ------------------------------------------------------------------ environment
@dataclass
class Env:
    tool: Path
    clang: str
    llvm_version: str
    out_dir: Path
    tu_timeout: int


def tool_llvm_version(tool: Path) -> str:
    try:
        out = subprocess.run([str(tool), "--version"], capture_output=True,
                             text=True, timeout=30).stdout
    except (OSError, subprocess.TimeoutExpired) as exc:
        die(f"error: cannot run {tool} --version: {exc}")
    m = re.search(r"LLVM version (\d+\.\d+\.\d+)", out)
    if not m:
        die(f"error: cannot read the LLVM version from {tool} --version:\n{out}")
    return m.group(1)


def clang_major(clang: str) -> int | None:
    try:
        out = subprocess.run([clang, "--version"], capture_output=True,
                             text=True, timeout=30).stdout
    except (OSError, subprocess.TimeoutExpired):
        return None
    m = re.search(r"clang version (\d+)", out)
    return int(m.group(1)) if m else None


def find_clang(major: int) -> str:
    """A clang whose builtin headers match the tool's LLVM."""
    for cand in (f"clang-{major}", f"/usr/lib/llvm-{major}/bin/clang", "clang"):
        if shutil.which(cand) and clang_major(cand) == major:
            return cand
    die(f"error: no clang {major} found (tried clang-{major}, "
                     f"/usr/lib/llvm-{major}/bin/clang, clang); pass --clang")


def make_env(args: argparse.Namespace) -> Env:
    tool = Path(args.tool).resolve()
    if not os.access(tool, os.X_OK):
        die(f"error: tool not found or not executable: {tool}\n"
                         "hint: build it first (see CLAUDE.md) or pass --tool")
    version = tool_llvm_version(tool)
    major = int(version.split(".")[0])
    clang = args.clang or find_clang(major)
    if clang_major(clang) != major:
        print(f"warning: {clang} is not clang {major}; builtin headers may not match",
              file=sys.stderr)
    return Env(tool, clang, version, Path(args.out_dir).resolve(), args.tu_timeout)


# --------------------------------------------------------------------- fixtures
@dataclass
class Result:
    """Outcome of one fixture run."""
    ok: bool                       # driver exited 0
    log: Path
    wall_ms: float
    files: dict[Path, str] = field(default_factory=dict)  # expected path -> normalized text
    metrics: dict[str, float] = field(default_factory=dict)


@dataclass
class Fixture:
    name: str
    kind: str                      # "example" | "project"
    expected_dir: Path
    runner: Callable[[Env], Result]

    def managed_expected(self) -> set[Path]:
        if self.kind == "example":
            p = self.expected_dir / f"{self.name}{EXPECTED_MARK}.txt"
            return {p} if p.exists() else set()
        if not self.expected_dir.is_dir():
            return set()
        return {p for p in self.expected_dir.rglob("*") if p.is_file() and is_managed_expected(p)}

    @property
    def runtime_path(self) -> Path:
        if self.kind == "example":
            return self.expected_dir / f"{self.name}.{RUNTIME_NAME}"
        return self.expected_dir / RUNTIME_NAME


def run_example(name: str, env: Env) -> Result:
    base = env.out_dir / "examples" / name
    shutil.rmtree(base, ignore_errors=True)
    src_dir = base / "src" / "examples"       # driver prints ../examples/<name>.c
    out_dir = base / "out"
    src_dir.mkdir(parents=True)
    shutil.copy2(EXAMPLES_DIR / f"{name}.c", src_dir / f"{name}.c")
    log = base / "driver.log"
    # Run from the tool's directory with a relative TOOL so the "Command:"
    # line in the output does not depend on where the build lives.
    cmd = ["bash", str(EXAMPLES_DRIVER)]
    environ = dict(os.environ, TOOL=f"./{env.tool.name}", EXAMPLES_DIR=str(src_dir),
                   OUTPUT_DIR=str(out_dir))
    t0 = time.perf_counter()
    with log.open("w") as fh:
        rc = subprocess.run(cmd, cwd=env.tool.parent, env=environ, stdout=fh,
                            stderr=subprocess.STDOUT).returncode
    wall = (time.perf_counter() - t0) * 1000
    res = Result(ok=rc == 0, log=log, wall_ms=wall, metrics={"wallMs": round(wall)})
    produced = out_dir / f"{name}-output.txt"
    if produced.exists():
        roots = [(REPO, "<tool-root>"), (env.out_dir, "<output-root>")]
        res.files[EXAMPLES_DIR / "expected" / f"{name}{EXPECTED_MARK}.txt"] = normalize(produced, roots)
    return res


def ensure_compdb(project: Path, env: Env) -> None:
    """CLAUDE.md: compile_commands.json must exist before a project is run."""
    if (project / "compile_commands.json").exists():
        return
    subprocess.run([sys.executable, str(COMPDB_SCRIPT), "--project-root", str(PROJECTS_DIR),
                    "--project-glob", project.name, "--cc", env.clang],
                   check=True, stdout=subprocess.DEVNULL)
    if not (project / "compile_commands.json").exists():
        die(f"error: could not create {project}/compile_commands.json")


def run_project(name: str, env: Env) -> Result:
    project = PROJECTS_DIR / name
    ensure_compdb(project, env)
    base = env.out_dir / "projects" / name
    shutil.rmtree(base, ignore_errors=True)
    base.parent.mkdir(parents=True, exist_ok=True)
    log = env.out_dir / "projects" / f"{name}.log"
    cmd = ["bash", str(PROJECT_DRIVER), "--tool", str(env.tool), "--clang", env.clang,
           "--normalize-paths", "--tu-timeout", str(env.tu_timeout), "--heartbeat", "0",
           "-o", str(base), str(project)]
    t0 = time.perf_counter()
    with log.open("w") as fh:
        rc = subprocess.run(cmd, cwd=REPO, stdout=fh, stderr=subprocess.STDOUT,
                            env=dict(os.environ, NO_COLOR="1")).returncode
    wall = (time.perf_counter() - t0) * 1000
    res = Result(ok=rc == 0, log=log, wall_ms=wall)
    out = base / name
    roots = [(project.resolve(), "<project-root>"), (env.out_dir, "<output-root>"),
             (REPO, "<tool-root>")]
    if out.is_dir():
        for path in sorted(out.rglob("*")):
            rel = path.relative_to(out)
            if path.is_file() and is_compared_project_file(rel):
                res.files[project / "expected" / expected_name(rel)] = normalize(path, roots)
    res.metrics = project_metrics(out, wall)
    return res


def project_metrics(out: Path, wall_ms: float) -> dict[str, float]:
    m: dict[str, float] = {"wallMs": round(wall_ms)}
    index = out / "project-summary-index.json"
    if index.exists():
        t = json.loads(index.read_text()).get("buildMetadata", {}).get("timingsMilliseconds", {})
        if "totalBeforeWrite" in t:
            m["indexBuildMs"] = t["totalBeforeWrite"]
    tu_total = 0
    for side in (out / "runtime-metrics").glob("*.cast-runtime.json") if out.is_dir() else []:
        tu_total += json.loads(side.read_text()).get("timingsMilliseconds", {}).get("totalElapsed", 0)
    m["tuTotalElapsedMs"] = tu_total
    return m


def all_fixtures() -> list[Fixture]:
    fx = [Fixture(n, "example", EXAMPLES_DIR / "expected", lambda e, n=n: run_example(n, e))
          for n in REGRESSION_EXAMPLES]
    fx += [Fixture(n, "project", PROJECTS_DIR / n / "expected", lambda e, n=n: run_project(n, e))
           for n in REGRESSION_PROJECTS]
    return fx


def select(only: list[str]) -> list[Fixture]:
    fixtures = all_fixtures()
    if not only:
        return fixtures
    picked = []
    for f in fixtures:
        if f.name in only or (f.kind + "s") in only:
            picked.append(f)
    unknown = set(only) - {f.name for f in fixtures} - {"examples", "projects"}
    if unknown:
        die(f"error: unknown fixture(s): {', '.join(sorted(unknown))}\n"
                         f"known: examples, projects, {', '.join(f.name for f in fixtures)}")
    return picked


# --------------------------------------------------------------------- commands
def rel(p: Path) -> str:
    try:
        return str(p.relative_to(REPO))
    except ValueError:
        return str(p)


def log_tail(log: Path, n: int = 15) -> str:
    lines = log.read_text(errors="replace").splitlines() if log.exists() else []
    return "\n".join("      " + l for l in lines[-n:])


def check_meta(fixtures: list[Fixture], env: Env) -> None:
    major = env.llvm_version.split(".")[0]
    for d in sorted({f.expected_dir for f in fixtures}):
        meta = d / META_NAME
        if meta.exists():
            want = json.loads(meta.read_text()).get("llvmVersion", "")
            if want.split(".")[0] != major:
                print(f"warning: {rel(meta)} was generated with LLVM {want}, "
                      f"tool uses LLVM {env.llvm_version}; differences may be version noise",
                      file=sys.stderr)


def cmd_run(args: argparse.Namespace, env: Env) -> int:
    fixtures = select(args.only)
    check_meta(fixtures, env)
    norm_root = env.out_dir / "normalized"
    passed = failed = 0
    for f in fixtures:
        res = f.runner(env)
        problems: list[str] = []
        if not res.ok:
            problems.append(f"driver failed; log: {rel(res.log)}\n{log_tail(res.log)}")
        expected = f.managed_expected()
        for exp_path, actual in sorted(res.files.items()):
            # Keep a normalized copy for inspection / manual cp.
            copy = norm_root / exp_path.relative_to(REPO)
            copy.parent.mkdir(parents=True, exist_ok=True)
            copy.write_text(actual)
            if not exp_path.exists():
                problems.append(f"MISSING expected file {rel(exp_path)}")
                continue
            want = exp_path.read_text(errors="replace")
            if want != actual:
                diff = list(difflib.unified_diff(
                    want.splitlines(), actual.splitlines(),
                    fromfile=rel(exp_path), tofile=rel(copy), lineterm="", n=3))
                if len(diff) > args.max_diff_lines:
                    diff = diff[:args.max_diff_lines] + [
                        f"... diff truncated ({len(diff)} lines); full output: {rel(copy)}"]
                problems.append("DIFF\n" + "\n".join("      " + l for l in diff))
        for stale in sorted(expected - set(res.files)):
            problems.append(f"STALE expected file (no longer produced) {rel(stale)}")
        if res.ok and not res.files:
            problems.append("driver produced no compared output")
        status = "PASS" if not problems else "FAIL"
        print(f"{status}  {f.kind:<7} {f.name}  ({res.wall_ms / 1000:.1f}s, {len(res.files)} files)")
        for p in problems:
            print("    " + p)
        passed += status == "PASS"
        failed += status == "FAIL"
    print(f"\nPASS {passed} / FAIL {failed}   (normalized output: {rel(norm_root)})")
    if failed:
        print("hint: if the change is intended, run `scripts/regress.py update` and review "
              "`git diff -- '*.expected*'`")
    return 1 if failed else 0


def require_llvm(env: Env) -> None:
    if int(env.llvm_version.split(".")[0]) < MIN_LLVM_MAJOR:
        die(f"error: tool is built with LLVM {env.llvm_version}; expected files "
                         f"must come from LLVM >= {MIN_LLVM_MAJOR}")


def write_meta(dirs: set[Path], env: Env) -> None:
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)
        (d / META_NAME).write_text(json.dumps(
            {"llvmVersion": env.llvm_version, "generator": "scripts/regress.py update"},
            indent=2, sort_keys=True) + "\n")


def cmd_update(args: argparse.Namespace, env: Env) -> int:
    require_llvm(env)
    fixtures = select(args.only)
    results = {f.name: f.runner(env) for f in fixtures}
    bad = [n for n, r in results.items() if not r.ok or not r.files]
    if bad:
        for n in bad:
            print(f"FAIL  {n}: log {rel(results[n].log)}\n{log_tail(results[n].log)}")
        print("error: not updating expected files because some fixtures failed", file=sys.stderr)
        return 1
    written = removed = unchanged = 0
    for f in fixtures:
        res = results[f.name]
        for path, text in res.files.items():
            if path.exists() and path.read_text(errors="replace") == text:
                unchanged += 1
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
            written += 1
            print(f"wrote   {rel(path)}")
        for stale in sorted(f.managed_expected() - set(res.files)):
            stale.unlink()
            removed += 1
            print(f"removed {rel(stale)}")
    write_meta({f.expected_dir for f in fixtures}, env)
    print(f"\n{written} written, {removed} removed, {unchanged} unchanged (LLVM {env.llvm_version})")
    return 0


def measure(f: Fixture, env: Env, runs: int) -> tuple[bool, dict[str, float]]:
    samples: list[dict[str, float]] = []
    for _ in range(runs):
        res = f.runner(env)
        if not res.ok:
            print(f"FAIL  {f.name}: log {rel(res.log)}\n{log_tail(res.log)}")
            return False, {}
        samples.append(res.metrics)
    keys = sorted(set().union(*samples))
    return True, {k: statistics.median(s[k] for s in samples if k in s) for k in keys}


def cmd_perf(args: argparse.Namespace, env: Env) -> int:
    fixtures = select(args.only)
    if args.update:
        require_llvm(env)
    regressions = errors = 0
    for f in fixtures:
        ok, metrics = measure(f, env, args.runs)
        if not ok:
            errors += 1
            continue
        if args.update:
            f.runtime_path.parent.mkdir(parents=True, exist_ok=True)
            f.runtime_path.write_text(json.dumps({
                "schemaName": "cast-history-regress-runtime-baseline",
                "schemaVersion": 1,
                "llvmVersion": env.llvm_version,
                "statistic": f"median of {args.runs} runs",
                "machine": {"system": platform.system(), "arch": platform.machine(),
                            "cpus": os.cpu_count()},
                "metricsMilliseconds": metrics,
            }, indent=2, sort_keys=True) + "\n")
            print(f"wrote   {rel(f.runtime_path)}  {metrics}")
            continue
        if not f.runtime_path.exists():
            print(f"NOBASE  {f.name}  {metrics}  (create with `regress.py perf --update`)")
            continue
        base = json.loads(f.runtime_path.read_text()).get("metricsMilliseconds", {})
        lines, slow = [], False
        for k, now in metrics.items():
            was = base.get(k)
            if was is None:
                continue
            limit = was * (1 + args.tolerance)
            flag = now > limit and now - was >= args.min_delta_ms
            slow |= flag
            lines.append(f"{k}={now:.0f}ms (baseline {was:.0f}ms){'  <-- SLOWER' if flag else ''}")
        regressions += slow
        print(f"{'SLOW' if slow else 'OK':<6}  {f.name}  " + ", ".join(lines))
    print(f"\n{regressions} slower than baseline (tolerance {args.tolerance:.0%}, "
          f"min delta {args.min_delta_ms}ms), {errors} failed")
    if errors or (args.strict and regressions):
        return 1
    return 0


# ------------------------------------------------------------------------- main
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1],
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog="See design/regression-testing.md.")
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--only", action="append", default=[], metavar="NAME",
                        help="fixture name, 'examples' or 'projects' (repeatable)")
    common.add_argument("--tool", default=str(REPO / "build" / "cast-history-tool"),
                        help="cast-history-tool binary (default: build/cast-history-tool)")
    common.add_argument("--clang", help="clang for builtin headers (default: clang-<tool LLVM major>)")
    common.add_argument("--out-dir", default=str(REPO / "build" / "regress-out"),
                        help="scratch output directory (default: build/regress-out)")
    common.add_argument("--tu-timeout", type=int, default=600,
                        help="per-TU analyzer time limit in seconds (default 600)")
    sub = ap.add_subparsers(dest="command", required=True)
    p_run = sub.add_parser("run", parents=[common], help="diff against expected files")
    p_run.add_argument("--max-diff-lines", type=int, default=200,
                       help="truncate each file's diff after N lines (default 200)")
    sub.add_parser("update", parents=[common], help="rewrite expected files")
    p_perf = sub.add_parser("perf", parents=[common], help="compare timings with baselines")
    p_perf.add_argument("--update", action="store_true", help="rewrite runtime baselines")
    p_perf.add_argument("--runs", type=int, default=3, help="runs per fixture; median is used")
    p_perf.add_argument("--tolerance", type=float, default=0.25,
                        help="allowed slowdown as a fraction (default 0.25)")
    p_perf.add_argument("--min-delta-ms", type=float, default=50,
                        help="ignore slowdowns smaller than this (default 50ms)")
    p_perf.add_argument("--strict", action="store_true", help="exit 1 on a slowdown")
    args = ap.parse_args()

    env = make_env(args)
    env.out_dir.mkdir(parents=True, exist_ok=True)
    print(f"tool: {rel(env.tool)} (LLVM {env.llvm_version})  clang: {env.clang}  "
          f"out: {rel(env.out_dir)}\n")
    return {"run": cmd_run, "update": cmd_update, "perf": cmd_perf}[args.command](args, env)


if __name__ == "__main__":
    sys.exit(main())

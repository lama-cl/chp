#!/usr/bin/env python3
"""Generate the human review packet for one pointer-coverage phase.

The gates prove a phase mechanically.  The packet is what a person reads to
decide whether the mechanism is *right*: fixture sources next to what the
tool now says about them, corpus numbers before and after, and a fixed,
reproducible sample of real-code cases to check against the source by eye.

  review_packet.py --phase P0 --tool build/cast-history-tool \
      --before-tool /path/to/previous/cast-history-tool

Writes review/<phase>.md.  The reviewer records the verdict in signoffs.json.
"""
import argparse
import concurrent.futures
import datetime
import difflib
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
import coverage_ledger  # noqa: E402
import run_corpus  # noqa: E402
import run_gates  # noqa: E402

SAMPLE_SIZE = 12


def rel(path):
    try:
        return str(pathlib.Path(path).resolve().relative_to(REPO))
    except (ValueError, OSError):
        return str(path)


def source_line(path, line):
    try:
        return pathlib.Path(path).read_text(errors="replace").splitlines()[line - 1].strip()
    except (OSError, IndexError):
        return "<source unavailable>"


def subject_cell(row):
    if row is None:
        return "no subject row"
    parts = [f"`{coverage_ledger.bucket_of(row)}`"]
    types = sorted(coverage_ledger.concrete_types(row))
    if types:
        parts.append("types: " + ", ".join(f"`{t}`" for t in types))
    escape = row.get("escape")
    if escape:
        text = "escape: " + "+".join(escape["reasons"])
        external = escape.get("externalCallees", escape["callees"])
        project = escape.get("projectCallees", [])
        if external:
            text += "; external " + ", ".join(f"`{c}`" for c in external)
        if project:
            text += "; project " + ", ".join(f"`{c}`" for c in project)
        parts.append(text)
    for key, label in (("unionMediation", "union"),):
        if row.get(key):
            parts.append(f"{label}: {row[key].get('kind')}")
    return "; ".join(parts)


def fixture_outputs(tool, gate, workdir):
    """(subjects by entity, indirect call sites) for one gate fixture."""
    run_gates.analyse(tool, [str(HERE / f) for f in gate["fixture"]], workdir, "c17")
    subjects, sites = {}, []
    for path in sorted((workdir / "import" / "paper-facts").glob("*.json")):
        doc = json.loads(path.read_text())
        for s in doc.get("subjects") or []:
            subjects.setdefault(s["entity"], s)
        sites.extend(doc.get("indirectCallSites") or [])
    return subjects, sites


def gate_section(gate, tool, before_tool, workroot, results):
    out = [f"### {gate['id']}: {gate['title']}", ""]
    status = results["gates"].get(gate["id"], "?")
    out.append(f"Gate status: **{status}**")
    out.append("")
    for fixture in gate["fixture"]:
        out.append(f"<details><summary><code>{rel(HERE / fixture)}</code></summary>")
        out.append("")
        out.append("```c")
        for n, text in enumerate((HERE / fixture).read_text().splitlines(), 1):
            out.append(f"{n:3d}  {text}")
        out.append("```")
        out.append("</details>")
        out.append("")
    after, sites = fixture_outputs(tool, gate, workroot / "after" / gate["id"])
    before = None
    if before_tool:
        try:
            before, _ = fixture_outputs(before_tool, gate, workroot / "before" / gate["id"])
        except run_gates.PipelineError as err:
            out.append(f"_before-tool failed on this fixture: {err}_")
    entities = []
    for check in gate["checks"]:
        if check["kind"] == "subject" and check["entity"] not in entities:
            entities.append(check["entity"])
    out.append("| probe subject | role | before | after |" if before is not None
               else "| probe subject | role | now |")
    out.append("|---|---|---|---|" if before is not None else "|---|---|---|")
    for entity in entities:
        roles = sorted({c["role"] for c in gate["checks"] if c.get("entity") == entity})
        cells = [f"`{entity}`", "/".join(roles)]
        if before is not None:
            cells.append(subject_cell(before.get(entity)))
        cells.append(subject_cell(after.get(entity)))
        out.append("| " + " | ".join(cells) + " |")
    out.append("")
    if sites:
        out.append("| indirect call | in | kind | tier | targets |")
        out.append("|---|---|---|---|---|")
        for s in sites:
            out.append(f"| `{s['calleeExpr']}` (line {s['location']['line']}) | `{s['function']}` | "
                       f"{s['calleeExprKind']} | {s['resolutionTier']} | "
                       f"{', '.join(s['targets']) or 'none'} |")
        out.append("")
    return out


def corpus_ledgers(tool, workroot, jobs, reuse=None):
    """Ledger per corpus project.  With `reuse`, read the import-round output
    that `run_corpus.py --keep <dir>` left there instead of rerunning."""
    ledgers = {}
    for project in json.loads((HERE / "corpus.json").read_text())["projects"]:
        if reuse and not (pathlib.Path(reuse) / project["name"] / "import").is_dir() and project.get("slow"):
            continue
        if not reuse and project.get("slow"):
            continue
        if reuse:
            facts = pathlib.Path(reuse) / project["name"] / "import" / "paper-facts"
            if not facts.is_dir():
                raise SystemExit(f"error: {facts} missing; run run_corpus.py --keep {reuse} first")
            ledger = coverage_ledger.build_ledger(facts)
            committed = HERE / "ledgers" / f"{project['name']}.json"
            if committed.exists():
                ledger["translationUnits"] = json.loads(committed.read_text())["translationUnits"]
            else:
                ledger["translationUnits"] = {"total": 0, "failed": 0, "failedNames": []}
            ledgers[project["name"]] = ledger
            continue
        workdir = workroot / project["name"]
        workdir.mkdir(parents=True, exist_ok=True)
        ledgers[project["name"]] = run_corpus.analyse(tool, project, workdir, jobs, 600)
    return ledgers


def example_text(tool, source):
    proc = subprocess.run([tool, str(source), "--", "-std=c17"], stdout=subprocess.PIPE,
                          stderr=subprocess.DEVNULL, text=True, timeout=600)
    return proc.stdout.replace(str(REPO) + "/", "")


def examples_section(tool, before_tool, jobs):
    """Text-output diff of the existing examples/ suite between two builds.
    Per decision 3 in the plan, every changed example is listed and reviewed
    with the phase that changed it."""
    out = ["## Changes to existing `examples/` output", ""]
    if not before_tool:
        return out + ["_No `--before-tool` given; not compared._", ""]
    sources = sorted((REPO / "examples").glob("*.c"))
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        before = list(pool.map(lambda s: example_text(before_tool, s), sources))
        after = list(pool.map(lambda s: example_text(tool, s), sources))
    changed = [(s, b, a) for s, b, a in zip(sources, before, after) if b != a]
    out.append(f"{len(changed)} of {len(sources)} examples changed their text output.")
    out.append("")
    for source, b, a in changed:
        diff = list(difflib.unified_diff(b.splitlines(), a.splitlines(), "before", "after", n=1, lineterm=""))
        out.append(f"<details><summary><code>{rel(source)}</code>: {sum(1 for l in diff if l[:1] in '+-') - 2} changed lines</summary>")
        out.append("")
        out.append("```diff")
        out.extend(diff[:80])
        if len(diff) > 80:
            out.append(f"... {len(diff) - 80} more diff lines")
        out.append("```")
        out.append("</details>")
        out.append("")
    return out


def corpus_section(after, before):
    out = ["## Corpus", "",
           "Source-only runs of the projects in `corpus.json` (C1, index, one import round). "
           "`blocked` counts subjects with no concrete type and a recorded reason; "
           "`opaque` counts subjects with neither.", ""]
    for kind, label in (("void-data-pointer", "`void *` subjects"),
                        ("nonvoid-data-pointer", "other data-pointer subjects")):
        out.append(f"**{label}**")
        out.append("")
        if before:
            out.append("| project | TUs ok | total | resolved (before -> after) | blocked (before -> after) | opaque (before -> after) |")
            out.append("|---|---|---|---|---|---|")
        else:
            out.append("| project | TUs ok | total | resolved | blocked | opaque |")
            out.append("|---|---|---|---|---|---|")
        for name, ledger in after.items():
            row = ledger["kinds"][kind]
            tus = ledger["translationUnits"]
            cells = [name, f"{tus['total'] - tus['failed']}/{tus['total']}", str(row["total"])]
            for field in ("resolved", "blocked", "opaqueNoEvidence"):
                if before:
                    old = before[name]["kinds"][kind][field]
                    cells.append(f"{old} -> {row[field]}")
                else:
                    cells.append(str(row[field]))
            out.append("| " + " | ".join(cells) + " |")
        out.append("")
    out.append("**Blocked subjects by reason** (all pointer kinds)")
    out.append("")
    reasons = sorted({b for l in after.values() for k in l["kinds"].values()
                      for b in k["buckets"] if b.startswith("blocked:")})
    out.append("| project | " + " | ".join(r.split(":", 1)[1] for r in reasons) + " | without a reason |")
    out.append("|---|" + "---|" * (len(reasons) + 1))
    for name, ledger in after.items():
        counts = [sum(k["buckets"].get(r, 0) for k in ledger["kinds"].values()) for r in reasons]
        out.append(f"| {name} | " + " | ".join(map(str, counts)) + f" | {ledger['blockedWithoutEscapeReason']} |")
    out.append("")
    out.append("**Indirect call sites** (kind/tier)")
    out.append("")
    keys = sorted({k for l in after.values() for k in l["indirectCallSites"]["byKindAndTier"]})
    out.append("| project | total | unresolved | " + " | ".join(keys) + " |")
    out.append("|---|---|---|" + "---|" * len(keys))
    for name, ledger in after.items():
        sites = ledger["indirectCallSites"]
        out.append(f"| {name} | {sites['total']} | {sites['unresolved']} | "
                   + " | ".join(str(sites["byKindAndTier"].get(k, 0)) for k in keys) + " |")
    out.append("")
    totals = {}
    for ledger in after.values():
        for row in ledger["escapeCallees"]["top"]:
            entry = totals.setdefault(row["callee"], [0, 0])
            entry[0] += row["subjects"]
            entry[1] += 1
    if totals:
        out.append("**External escape callees, ranked by subjects lost** (input to the P4 model list)")
        out.append("")
        out.append("| callee | subjects | projects |")
        out.append("|---|---|---|")
        for callee, (n, projects) in sorted(totals.items(), key=lambda kv: (-kv[1][0], kv[0]))[:25]:
            out.append(f"| `{callee}` | {n} | {projects} |")
        out.append("")
    return out


def sample_section(workroot, focus):
    """A fixed sample of real-code cases, chosen by content hash so that the
    same tool output always yields the same sample."""
    candidates = []
    for path in sorted(workroot.glob("*/import/paper-facts/*.json")):
        project = path.parts[-4]
        doc = json.loads(path.read_text())
        if focus == "indirect-call":
            for s in doc.get("indirectCallSites") or []:
                loc = s["location"]
                candidates.append((project, f"{loc['file']}:{loc['line']}:{loc['column']}", ("site", s)))
            continue
        for s in doc.get("subjects") or []:
            if (s.get("declaredType") or {}).get("outerPointerKind") not in coverage_ledger.POINTER_KINDS:
                continue
            keep = {"escape": bool(s.get("escape")),
                    "union": bool(s.get("unionMediation")),
                    "resolved": coverage_ledger.bucket_of(s) == "resolved"}.get(focus, False)
            if keep:
                candidates.append((project, s["subjectId"], ("subject", s)))
    unique = {(p, k): v for p, k, v in candidates}
    ranked = sorted(unique.items(), key=lambda kv: hashlib.sha1(repr(kv[0]).encode()).hexdigest())
    picked, per_project = [], {}
    for (project, _), value in ranked:  # spread the sample over projects
        if per_project.get(project, 0) >= max(2, SAMPLE_SIZE // 4):
            continue
        per_project[project] = per_project.get(project, 0) + 1
        picked.append((project, value))
        if len(picked) == SAMPLE_SIZE:
            break
    out = ["## Spot-check sample", "",
           f"{len(picked)} of {len(unique)} candidate cases (focus: {focus}), selected by content hash. "
           "Open each location and mark the verdict.", ""]
    for i, (project, (kind, item)) in enumerate(picked, 1):
        loc = item["location"]
        out.append(f"**{i}. {project}** `{rel(loc['file'])}:{loc['line']}`")
        out.append("")
        out.append("```c")
        out.append(source_line(loc["file"], loc["line"]))
        out.append("```")
        if kind == "site":
            out.append(f"- call `{item['calleeExpr']}` in `{item['function']}`: {item['calleeExprKind']}, "
                       f"tier {item['resolutionTier']}, targets: {', '.join(item['targets']) or 'none'}")
        else:
            out.append(f"- subject `{item['entity']}` : `{item['declaredType']['text']}` -> {subject_cell(item)}")
            for site in (item.get("escape") or {}).get("sites", [])[:3]:
                sloc = site["location"]
                out.append(f"- {site.get('kind', 'site')} `{site['callee']}` at `{rel(sloc['file'])}:{sloc['line']}`: "
                           f"`{source_line(sloc['file'], sloc['line'])}`")
        out.append("- [ ] correct  - [ ] wrong (say why):")
        out.append("")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--phase", required=True)
    ap.add_argument("--tool", default=os.environ.get("CAST_HISTORY_TOOL"), required=False)
    ap.add_argument("--before-tool", help="previous build, for before/after columns")
    ap.add_argument("--skip-corpus", action="store_true")
    ap.add_argument("--corpus-after", help="reuse run_corpus.py --keep output of --tool")
    ap.add_argument("--corpus-before", help="reuse run_corpus.py --keep output of --before-tool")
    ap.add_argument("--no-corpus-before", action="store_true",
                    help="show corpus numbers for --tool only (no before columns)")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--out", help="default review/<phase>.md")
    args = ap.parse_args()
    manifest = json.loads((HERE / "gates.json").read_text())
    if args.phase not in manifest["phases"]:
        ap.error(f"unknown phase {args.phase}")
    if not args.tool or not os.access(args.tool, os.X_OK):
        ap.error("pass --tool /path/to/cast-history-tool")
    tool = str(pathlib.Path(args.tool).resolve())
    before_tool = str(pathlib.Path(args.before_tool).resolve()) if args.before_tool else None
    phase = manifest["phases"][args.phase]
    gates = [g for g in manifest["gates"] if g["phase"] == args.phase]

    with tempfile.TemporaryDirectory(prefix="ptrcov-review-") as tmp:
        workroot = pathlib.Path(tmp)
        results_path = workroot / "gates.json"
        proc = subprocess.run([sys.executable, str(HERE / "run_gates.py"), "--tool", tool,
                               "--json", str(results_path)], stdout=subprocess.PIPE, text=True)
        results = json.loads(results_path.read_text())
        green = sum(1 for g in gates if results["gates"].get(g["id"]) == "GREEN")
        commit = subprocess.run(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD"],
                                stdout=subprocess.PIPE, text=True).stdout.strip()
        dirty = bool(subprocess.run(["git", "-C", str(REPO), "status", "--porcelain", "--untracked-files=no"],
                                    stdout=subprocess.PIPE, text=True).stdout.strip())
        doc = [f"# Review packet: {args.phase} {phase['title']}", "",
               f"Generated {datetime.date.today().isoformat()} by `review_packet.py` from commit "
               f"`{commit}`{' plus uncommitted changes' if dirty else ''}. Do not edit by hand, "
               "except to tick the checkboxes while reviewing.", "",
               "| | |", "|---|---|",
               f"| Phase gates green | {green} of {len(gates)} |",
               f"| All gates green | {sum(1 for s in results['gates'].values() if s == 'GREEN')} of {len(results['gates'])} |",
               f"| Regressions against baseline | {len(results['regressions'])} |",
               f"| Before/after comparison | {'yes' if before_tool else 'no (single build)'} |", "",
               *(["## Agent notes", "",
                   "Written by the implementing agent; check them like anything else.", ""]
                  + [f"- {n}" for n in phase.get("agentNotes", [])] + [""]
                  if phase.get("agentNotes") else []),
               "## How to review", "",
               "1. Read the fixture tables: does `after` say what the fixture's header comment asks for, "
               "and did nothing else change?",
               "2. Read the corpus tables: are the movements plausible in size and direction?",
               "3. Work through the spot-check sample against the source.",
               "4. Answer the questions at the end, then record the verdict in "
               "`gates/pointer-coverage/signoffs.json`.", "",
               "## Fixtures", ""]
        for gate in gates:
            doc += gate_section(gate, tool, before_tool, workroot / "fixtures", results)
        context = [g for g in manifest["gates"] if g["id"] in phase.get("contextGates", [])]
        if context:
            doc += ["## Fixtures of later phases (context)", "",
                    "These gates are expected to be RED. They are shown because this phase changes "
                    "what the tool records about them.", ""]
            for gate in context:
                doc += gate_section(gate, tool, before_tool, workroot / "context", results)
        doc += examples_section(tool, before_tool, args.jobs)
        if not args.skip_corpus:
            after_root = pathlib.Path(args.corpus_after) if args.corpus_after else workroot / "corpus-after"
            after = corpus_ledgers(tool, after_root, args.jobs, reuse=args.corpus_after)
            before = None
            if before_tool and not args.no_corpus_before:
                before = corpus_ledgers(before_tool, workroot / "corpus-before", args.jobs,
                                        reuse=args.corpus_before)
            doc += corpus_section(after, before)
            doc += sample_section(after_root, phase["reviewFocus"])
        doc += ["## Review questions", ""]
        doc += [f"- [ ] {q}" for q in phase["reviewQuestions"]]
        doc += ["", "## Verdict", "",
                "Record in `signoffs.json` under this phase: `status` (`approved` or `changes-requested`), "
                "`reviewer`, `date`, `notes`. Sealing the phase in `CMakeLists.txt` requires `approved`.", ""]
        out = pathlib.Path(args.out) if args.out else HERE / "review" / f"{args.phase}.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        text = "\n".join(doc).replace(str(REPO) + "/", "")
        out.write_text(text)
        print(f"wrote {rel(out)} ({len(text.splitlines())} lines); gates: {proc.stdout.strip().splitlines()[-1] if proc.stdout.strip() else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Regenerate compile_commands.json for tiny multi-TU test projects.

The test tree currently has two project sets by default:

  test_projects/projects
  test_projects/c3a_regressions

Each immediate child directory under those sets is treated as one independent
mini-project.  For every .c/.cc/.cpp file in a mini-project, this script writes
one compile_commands.json entry using a simple Clang-compatible command.

Typical use from the repository root:

  python3 scripts/regenerate_compile_commands.py

Useful options:

  python3 scripts/regenerate_compile_commands.py \
      --project-root test_projects \
      --project-sets projects c3a_regressions

  python3 scripts/regenerate_compile_commands.py \
      --project-glob c3a_regressions

  python3 scripts/regenerate_compile_commands.py \
      --cc clang \
      --std c11 \
      --extra-arg -Iinclude

The generated commands are intentionally simple because these test projects are
small and mostly self-contained.  If a project needs custom include paths, pass
--extra-arg repeatedly.
"""

from __future__ import annotations

import argparse
import json
import shlex
from pathlib import Path
from typing import Iterable, List


DEFAULT_PROJECT_SETS = ["projects", "c3a_regressions","c4a_dependency_edges"]


def source_files(project: Path) -> List[Path]:
    suffixes = {".c", ".cc", ".cpp", ".cxx"}
    return sorted(
        p for p in project.iterdir()
        if p.is_file() and p.suffix.lower() in suffixes
    )


def iter_projects(project_root: Path,
                  project_sets: Iterable[str],
                  project_globs: Iterable[str]) -> List[Path]:
    projects: List[Path] = []

    if project_globs:
        for glob_text in project_globs:
            for root in sorted(project_root.glob(glob_text)):
                if root.is_dir() and (root / "compile_commands.json").exists():
                    projects.append(root)
                elif root.is_dir():
                    projects.extend(
                        sorted(p for p in root.iterdir() if p.is_dir())
                    )
        return sorted(dict.fromkeys(projects))

    for set_name in project_sets:
        root = project_root / set_name
        if not root.exists():
            print(f"warning: project set not found, skipping: {root}")
            continue
        projects.extend(sorted(p for p in root.iterdir() if p.is_dir()))

    return sorted(dict.fromkeys(projects))


def language_for(src: Path) -> str:
    suffix = src.suffix.lower()
    if suffix == ".c":
        return "c"
    return "c++"


def std_flag(src: Path, c_std: str, cxx_std: str) -> str:
    return f"-std={c_std}" if language_for(src) == "c" else f"-std={cxx_std}"


def compile_command(project: Path,
                    src: Path,
                    compiler: str,
                    c_std: str,
                    cxx_std: str,
                    extra_args: List[str]) -> dict:
    # Use an argv-style command list.  Clang tooling accepts either `command`
    # or `arguments`; `arguments` avoids shell quoting ambiguity.
    #
    # IMPORTANT:
    #   The compile-command `directory` is the mini-project directory.
    #   Therefore the source path inside `arguments` must be meaningful relative
    #   to that directory.  Using str(src) here would produce paths such as:
    #
    #     test_projects/projects/p01_direct_cross_tu/a.c
    #
    #   which become invalid when interpreted relative to:
    #
    #     test_projects/projects/p01_direct_cross_tu
    #
    #   Use the project-local file name instead.
    src_arg = src.name
    obj_arg = src.name + ".o"
    args = [
        compiler,
        std_flag(src, c_std, cxx_std),
        "-I.",
        *extra_args,
        "-c",
        src_arg,
        "-o",
        obj_arg,
    ]
    return {
        "directory": str(project.resolve()),
        "file": str(src.resolve()),
        "arguments": args,
    }


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Regenerate compile_commands.json for tiny multi-TU test projects."
    )
    ap.add_argument(
        "--project-root",
        default="test_projects",
        help="Root containing project-set directories; default: test_projects",
    )
    ap.add_argument(
        "--project-sets",
        nargs="+",
        default=DEFAULT_PROJECT_SETS,
        help=(
            "Project-set directories under --project-root. "
            "Default: projects c3a_regressions"
        ),
    )
    ap.add_argument(
        "--project-glob",
        action="append",
        default=[],
        help=(
            "Glob relative to --project-root. May point to a project set or "
            "to project directories. Can be repeated. When set, --project-sets "
            "is ignored."
        ),
    )
    ap.add_argument(
        "--cc",
        default="clang",
        help="Compiler executable to record; default: clang",
    )
    ap.add_argument(
        "--std",
        default="c11",
        help="C language standard; default: c11",
    )
    ap.add_argument(
        "--cxx-std",
        default="c++17",
        help="C++ language standard; default: c++17",
    )
    ap.add_argument(
        "--extra-arg",
        action="append",
        default=[],
        help="Extra compiler argument to include. Can be repeated.",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be written without modifying files.",
    )
    ap.add_argument(
        "--verbose",
        action="store_true",
        help="Print each generated command.",
    )

    args = ap.parse_args()

    project_root = Path(args.project_root)
    if not project_root.exists():
        raise SystemExit(f"error: project root not found: {project_root}")

    projects = iter_projects(project_root, args.project_sets, args.project_glob)
    if not projects:
        print(f"warning: no projects found under {project_root}")
        return 0

    written = 0
    skipped = 0

    for project in projects:
        srcs = source_files(project)
        if not srcs:
            print(f"skip: {project} (no source files)")
            skipped += 1
            continue

        entries = [
            compile_command(
                project=project,
                src=src,
                compiler=args.cc,
                c_std=args.std,
                cxx_std=args.cxx_std,
                extra_args=args.extra_arg,
            )
            for src in srcs
        ]

        out = project / "compile_commands.json"
        if args.verbose:
            print(f"{project}: {len(entries)} source file(s)")
            for entry in entries:
                print("  " + " ".join(shlex.quote(x) for x in entry["arguments"]))

        if args.dry_run:
            print(f"would write: {out} ({len(entries)} entr{'y' if len(entries) == 1 else 'ies'})")
        else:
            out.write_text(json.dumps(entries, indent=2) + "\n")
            print(f"wrote: {out} ({len(entries)} entr{'y' if len(entries) == 1 else 'ies'})")
            written += 1

    print()
    if args.dry_run:
        print(f"dry run complete: {len(projects)} project(s), skipped {skipped}")
    else:
        print(f"done: wrote {written} compile_commands.json file(s), skipped {skipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

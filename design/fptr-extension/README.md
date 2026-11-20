# Function-pointer (fptr) extension

This extends Cast History so that `void*` and other data pointers that flow through **function pointers** can be classified. Resolution quality is reported through a new facet, alongside the existing 7 primary categories.

**Status:** M0 (evidence and design) is done. No analyzer code has changed yet. Next milestone: **M1** (resolution facet, reporting only).

## Read in this order

| Doc | For | Content |
|---|---|---|
| [REVIEW.md](REVIEW.md) | humans, ~5 min | discussion summary, owner decisions, pushback and concerns, open questions |
| [00-challenges.md](00-challenges.md) | everyone | why fptrs limit classification, with measured gaps and 3 soundness bugs |
| [01-fact-catalog.md](01-fact-catalog.md) | everyone | the 48 facts (F01–F35, X01–X13) plus 9 documented-only patterns, with real-world citations |
| [02-design.md](02-design.md) | implementers | slots, resolver layers, data/control-flow tracking, cross-TU, facet, diagrams |
| [03-decisions.md](03-decisions.md) | reviewers | ADR log D1–D16 with status |
| [04-implementation-plan.md](04-implementation-plan.md) | agents | milestones M1–M9, the fixtures each claims, exact commands, definition of done |
| [evidence/evidence-table.md](evidence/evidence-table.md) | everyone | generated table: baseline today vs expected, per subject |

## Fixtures and evidence

- Single TU: `examples/fptr-NN-<fact>.c` (fact IDs F01–F35).
- Cross-TU: `test_projects/projects/p1NN_fptr_xtu_<fact>/` (fact IDs X01–X13), with `expected/README.expected.txt`.
- Each fixture's header comment carries the contract:

  ```c
  // @fptr-fact F05 const-dispatch-table
  // @subject   run::ctx
  // @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback ...   <- today, pinned
  // @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1 <- required
  ```

```bash
# build once
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=$(llvm-config --cmakedir) \
      -DClang_DIR=$(llvm-config --libdir)/cmake/clang -G Ninja && ninja -C build
# today's output still matches the pinned baselines (regression guard)
python3 scripts/check_fptr_evidence.py --check baseline
# milestone acceptance
python3 scripts/check_fptr_evidence.py --check expected --only F08 F09 F10 X11 X12
# regenerate the evidence table
python3 scripts/check_fptr_evidence.py --markdown design/fptr-extension/evidence/evidence-table.md
```

At M0: 48 fixtures, 50 subjects, baseline drift 0. All `@expected` are `xfail` because the `resolution` facet does not exist yet. Six fixtures (F01, F02, F03, F14, F25, F31) already have the right category and types. Five subjects (F09, F10, F35, X11, X12) are wrong today.

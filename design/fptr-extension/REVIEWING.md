# Reviewing guide (about 10 minutes)

## What to read

| You are… | Read | Time |
|---|---|---|
| the owner, deciding | [REVIEW.md](REVIEW.md) §6 open questions → [05-plan-review.md](05-plan-review.md) §2 and §6 | 5 min |
| reviewing a milestone PR | the PR's task card → [06-backbone-contract.md](06-backbone-contract.md) §2 (seams) → the diff → the evidence-table diff | 10 min |
| new to the design | [README.md](README.md) → [00-challenges.md](00-challenges.md) → [02-design.md](02-design.md) | 30 min |

## Three commands

```bash
(cd build && ctest -L fptr --output-on-failure)                        # every fixture still matches its pinned baseline
python3 scripts/check_fptr_evidence.py --check expected --only F36 F08  # the PR's claimed fixtures now meet @expected
python3 scripts/check_fptr_evidence.py --only F36 --show-observed       # see what one fixture produces
```

## Red flags in a milestone PR

- [ ] A fixture that is **not** in the claim list changed its `@baseline`.
- [ ] An `@expected` line was edited in the same commit as code. Expectations change only in their own commit, with a reason in REVIEW.md.
- [ ] `CastHistoryVisitor.h` or `main.cpp` changed outside a seam listed in the task card, or beyond the diff budget.
- [ ] A non-fptr state variable appears in the diff (`originState`, `valueFlows`, `aliasParentState`, …).
- [ ] A new multi-target set is printed in a different order on a second run (D21).
- [ ] The owner gate of the milestone is not met, for example M2 before D8 is accepted.
- [ ] No dated entry in REVIEW.md, or no regenerated `evidence/evidence-table.md`.

## What a good milestone PR looks like

- Title: `<Mx>: <descriptive name>` (CLAUDE.md PR rule).
- Small: the claimed fixtures flip from `xfail` to `expected-met`, the seams are named, and the visitor diff is within budget.
- The evidence-table diff shows exactly the claimed rows changing.
- REVIEW.md has an entry: what changed, any deviations, any new open question.

## LLVM version (done 2026-11-25: LLVM 23.1.4)

The fptr baselines were verified on LLVM 23.1.4 with **zero drift** (56 fixtures, 65 subjects). The version is recorded in `examples/fptr-meta.info`. To build on LLVM 23 from a clean shell:

```bash
unset C_INCLUDE_PATH CPLUS_INCLUDE_PATH     # these point at clang 18 headers and break LLVM 23
L=/opt/llvm-23/bin
rm -rf build/CMakeCache.txt build/CMakeFiles  # the compiler changes, so the cache must be fresh
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=$($L/llvm-config --cmakedir) \
      -DClang_DIR=$($L/llvm-config --libdir)/cmake/clang \
      -DCMAKE_C_COMPILER=$L/clang -DCMAKE_CXX_COMPILER=$L/clang++ -G Ninja && ninja -C build
(cd build && ctest -L fptr --output-on-failure)
```

Pitfalls found while doing this. Each one is fixed or guarded in the repo:

| Symptom | Cause | Fix |
|---|---|---|
| `<cstdint>`: no member `int8_t` | the shell profile sets `C_INCLUDE_PATH`/`CPLUS_INCLUDE_PATH` to clang 18's builtin headers | unset them (in the profile, or per shell) |
| undefined `std::string` symbols (`Ss`) at link time | CMake was configured while the variables above were set; LLVM's ABI probe failed and fell back to the old ABI | configure fresh with them unset |
| `getBreakLoc`/`getContinueLoc` missing | removed from newer Clang | `getBeginLoc()`, same location, works on 18 and 23 (`CastHistoryVisitor.h` `VisitBreakStmt`/`VisitContinueStmt`, not a seam: build fix only) |
| undefined `typeinfo for llvm::cl::GenericOptionValue` | LLVM 23 is built `-fno-rtti`; Ubuntu LLVM 18 is built with RTTI | `CMakeLists.txt` follows `LLVM_ENABLE_RTTI` |
| `'stddef.h' file not found` when analysing C | LibTooling looks for builtin headers next to `build/cast-history-tool` | the tool defaults to `-resource-dir=${LLVM_LIBRARY_DIR}/clang/<major>` (`CAST_HISTORY_DEFAULT_RESOURCE_DIR`); an explicit `-resource-dir` still wins |

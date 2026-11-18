# CLAUDE.md for LLVM Analyzers

## Project Overview
* **What**: "Type analysis for C void pointers using typecasts, operation profiles and pointer flows built from walking clang AST."
* **Target LLVM Version**: LLVM 19+ (verify via `llvm-config --version`)
* **Language**: C++20 / C++17
* **Compiler**: clang++

## Build & Test Commands
* **Configure (CMake)**:
  ```bash
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=\$(llvm-config --cmakedir) -G Ninja
  ```
* **Build**:
  ```bash
  ninja -j\$(nproc)
  ```
* **Run Tests**:
  ```bash
  cd build && ctest --output-on-failure
  # Or running lit tests directly:
  lit test/
  ```

## Codebase Structure
* `include/` — Public headers for analysis passes and visitors
* `scripts/` — Standalone CLI drivers or wrappers
* `examples/` — Minimal single-source C examples to demonstrate features or properties of programs  covered by the analyzer
* `test_projects/` — Minimal multi-tu C examples to demonstrate features or properties of projects  covered by the analyzer; before running tests on any test_project, its `compile_commands.json` must be built.

## Development Rules & Conventions
* Always run the whole `examples` & `test_projects` fixtures before PR creation to avoid any regressions.
* Follow the [LLVM Coding Standards](https://llvm.org).
* Use LLVM's `llvm::raw_ostream` instead of `std::cout`.
* Prefer `llvm::ilist`, `llvm::SmallVector`, and `llvm::DenseMap` over standard STL containers where performance or memory matters.
* Keep analysis passes immutable (`run(Function &F, FunctionAnalysisManager &AM)`), and query the analysis manager properly.
* Update or add `FileCheck` regression tests in `test/` whenever modifying analysis logic or output formatting.

### PRs
* All PRs must be named with either issue name or a descriptive name that describes the work in the PR.

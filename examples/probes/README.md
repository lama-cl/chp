# Exploratory fptr probes (not fixtures)

These files are the owner's original function-pointer probes. They are kept **byte-identical**: moved here from `examples/` on 2026-11-25, with sha256 verified. They are not gated:

- `scripts/check_fptr_evidence.py` only reads `examples/fptr-NN-*.c`.
- `scripts/run_all_examples_normalized.sh` only reads `examples/*.c`, not subfolders.

Seven of the eight probes do not compile as C, so their `.history` files describe clang's *recovered* AST and are not evidence. Known problems:

- a missing `struct` tag (`X x1`);
- a trailing comma in a member list (`int i,`);
- `FPTR_t *fn` (pointer to fptr) where `FPTR_t fn` was meant;
- calls `x1.fn()` that pass no argument.

They also share a header copied from another example. Check with `clang -fsyntax-only -std=c11 <file>`.

Each probe idea that the existing fixtures did not cover was rewritten as a valid, marker-carrying fixture:

| Probe | Idea | Covered by |
|---|---|---|
| `fptr-call-reassign-call.c` (+ duplicate `fptr-call-reassign-call.history`) | local fptr reassigned between two calls | **F38** `fptr-38-strong-update-between-calls.c` (control) |
| `fptr-2.c`, `fptr-hof-reassign-hof.c` (identical) | fptr reassigned between two higher-order calls | **F39** `fptr-39-hof-reassign-between-calls.c` (control) |
| `fptr-struct-first-member.c` | fptr inside a nested record shared by two outer records | **F40** (local, control); **F37** (installed and called in different functions, a coverage gap that also motivates D7′) |
| `fptr-struct-multiple.c` | two fptr slots (read/write) per nested record | **F40** (control) |
| `fptr-struct-subtype-siblings.c` | `(*x1.fn)()` explicit-dereference call form; sibling record types | **F41** (control); **F42** (base-prefix cast, the soundness guard for D7′) |
| `fptr-1.c` | sibling record types, each with an fptr member | already covered by F15/F30 (record slots) and F41 |
| `fptr-struct-member.c` | anonymous union of data next to an fptr | already covered by F25 (union) and F40; no new fptr fact |

Review round 2 also added **F36**: a one-armed `if` after a multi-target merge, which is a soundness bug. It came from generalising the reassign probe. See `design/fptr-extension/05-plan-review.md`.

Struct probes that call `s.fn()` can be read three ways. Each reading is a fixture:

| Reading | Example | Fixtures |
|---|---|---|
| the instance's own sibling member | `s.fn(s.arg)` | F18, F20 |
| the whole object as `self` | `s.fn(&s)` | F16 |
| a separate caller-side `void *ctx` | `s.fn(ctx)` | F15, F30, F37, F40, F41 |

The third is the most common idiom in real C code; ops tables and handler structs use it.

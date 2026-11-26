# 00 — Why function pointers limit void-pointer classification

**Question 0:** what makes function pointers (fptrs) hard for the cast-history analyzer, so that pointers involved with them are not classified reliably?

Everything in this document was measured on commit `875f9a54` (LLVM 18.1.3 build), before any analyzer change. Every claim is pinned by a fixture. Re-check with:

```bash
python3 scripts/check_fptr_evidence.py --check baseline
```

## How fptrs reach the void* categories

A data pointer `ctx` gets its primary category from the cast and operation evidence attached to it (`paperPrimaryCategory`, `CastHistoryVisitor.h:21200`). When `ctx` is passed to an indirect call `cb(ctx)`, the evidence is in the **callee's body**. The caller sees it only if the analyzer knows which function `cb` points to. When it doesn't, that evidence is missing from `ctx`'s history, and `ctx` falls back to `OpaqueTransportOrViewOnly` (with `CallbackDependentPointer` behavior).

So fptr coverage is a **target-resolution problem**: find the functions an fptr expression may hold at a call. Two flow problems sit on top of it:

- **Forward flow:** `ctx` is passed into the targets.
- **Backward flow:** values returned from the targets, plus callback/context pairs.

## What exists today

| Mechanism | Where | Scope |
|---|---|---|
| Flow-sensitive target per local fptr variable (`functionPointerTargets`, `functionPointerTargetSets`) | `CastHistoryVisitor.h:24088` | one function body; saved/restored per function (`:1877`) |
| `if/else` merge into a target set | `mergeFunctionPointerStates` `:25352` | `if` only |
| Field targets keyed by *syntactic base* (`ops.run`, `ctx->h`) | `recordFunctionPointerFieldTargetFact` `:25039` | same base declaration in the same function |
| Higher-order formals `$k.$i`, specialized at direct call sites | `resolveHigherOrderBindingForSnapshot` `:25541` | named formal parameters |
| Cross-TU import of known / multi-target summaries | `recordCrossTUFunctionPointerCallLookup` `:5553`, `...TargetSetLookup` `:5377` | targets that are already resolved locally |
| Expression forms understood as an fptr value | `resolveFunctionValue` `:25422` | `DeclRefExpr`, `&f`, `MemberExpr` (+ casts between function types) |

## Measured gaps

✅ = matches the required behavior · ❌ = coverage loss · ⚠️ = **wrong answer** (unsound)

| # | Pattern | Today | Root cause |
|---|---|---|---|
| F01/F02/F25 | local var, null-guarded call, union member | ✅ | — |
| F03 | global with initializer | ✅ *by accident* | initializer happens to be traversed before the caller |
| F34 | same global defined after use | ❌ | resolution depends on declaration order |
| **F35** | global initialized, reassigned in another function | ⚠️ `int*` only | the file-scope initializer is treated as the current value inside every function |
| F04, F32, X02 | global assigned in a setter / constructor | ❌ | fptr state never crosses function boundaries |
| F05, F06, F33, X01 | dispatch tables `tbl[i](x)` | ❌ | `resolveFunctionValue` has no `ArraySubscriptExpr` case |
| F07 | `{name, fn}` table walked by a loop | ❌ (mediation says *Resolved*!) | element storage + loop pointer; the mediation label is set from provenance, not from a transfer |
| F08 | `c ? f : g` | ❌ | no `ConditionalOperator` case; `?:` is only syntactically traversed (`:3261`) |
| **F09, X11** | targets assigned in `switch` arms | ⚠️ `double*` only | `TraverseSwitchStmt` (`:3221`) is syntactic, so the last store wins |
| **F10, X12** | target reassigned on a loop back edge | ⚠️ `int*` only | no loop fixpoint (`:3231`–`:3259`) |
| F11, X04 | fptr returned by a factory | ❌ | no return-target summary |
| F12 | fptr written through `handler_t *out` | ❌ | no out-parameter target summary |
| F13 | fptr → `void*` → fptr | ❌ | the higher-order binding is lost at the `void*` |
| F14 | call through an incompatible signature | ✅ but the undefined behaviour is not flagged | no signature-mismatch fact |
| F15, F30, X03 | vtable `o->vt->m(o->data)`, fn field in a heap object | ❌ | field targets are keyed by syntactic base, not by record type |
| F16, X08 | OOP `void *self` downcast in a method | ❌ | same as above |
| F17, X07 | FSM: state returns the next state | ❌ | return targets + loop-carried fptr |
| F18, X05 | callback + context registration list | ❌ | storage of `(fn,arg)` pairs; no callback-context binding |
| F19, X06 | two registrations with different types | ❌ (risk ⚠️ once fixed naively) | crossing the pairs would produce a false multi-type result |
| F20, X13 | `struct job {fn,arg}` executor | ❌ (mediation says *Resolved*) | the struct-relative summary is not specialized |
| F21, X09 | `pthread_create(.., worker, ctx)` | ❌ `ExternalUnknown` | library-invoked callback not modelled |
| F22 | libc `qsort(base,.., cmp)` | ❌ `ExternalUnknown` | same, plus an element-address relation |
| F23, X10 | `dlsym` | ❌ unresolved (should be external) | unresolved and external are indistinguishable |
| F24 | indirect call returning its argument | ❌ | return-alias continuation is not applied to indirect calls (it works for direct calls) |
| F26 | `memcpy` of an ops struct | ❌ | memcpy is not a struct copy for fptr fields |
| F27 | recursion only through a table | ❌ | the table call `ops[i](…)` is never resolved, so **no call edge exists** and `op_rec` is an acyclic SCC. *Corrected in round 3:* resolved field-fptr calls and imported fptr targets already are SCC edges; resolved local-variable fptr calls are not ([07](07-existing-engine.md#4-indirect-calls-and-the-scc-graph-measured)) |
| F28 | fptr via `va_arg` | ❌ | not a named formal |
| F29 | `if (cb == use_int) cb(x)` | ❌ | no equality refinement |
| F31 | callback formal in a generic helper | ✅ (parametric) | must stay parametric, not be counted as a failure |

**Totals at M0:** 48 fixtures, 50 subjects. 6 already have the required category and types: F01, F02, F03 (by traversal order only), F14 (without the mismatch flag), F25 and F31. 5 subjects get a **wrong** answer today: F09, F10, F35, X11 and X12. The full matrix is in [evidence/evidence-table.md](evidence/evidence-table.md).

## The underlying challenges

1. **Target identity is a storage (points-to) problem.** fptrs live in globals, arrays, struct slots, unions, return values and out-parameters. These outlive one function body, and today's state does not.
2. **Flow-sensitivity without a CFG.** No CFG is built ([07 §2](07-existing-engine.md#2-intra-procedural-flow-no-cfg)). Only `if/else` snapshots and merges state; `switch`, loops, `?:` and `&&`/`||` are traversed once in AST order; `return`/`break`/`continue` do not end a branch's state. For data-pointer origins the approximation is tolerable. For fptr state it is **unsound** where a strong update in AST order deletes a reachable target (F09, F10, F35, F36), and **imprecise** where an exited branch is still joined (F44).
3. **The caller ↔ callee link is the evidence.** An unresolved call does not merely lose precision. It removes all of the callee's operations from the caller's history, so `ctx` *looks* like pure transport.
4. **Pairing.** A callback and its `void*` context are stored together and reunited later. Tracking each half separately crosses the pairs (F19), so the pairing must be preserved.
5. **External callers.** libc/POSIX functions invoke the callbacks themselves (`pthread_create`, `qsort`), and `dlsym` targets are not in the project. Neither is an analysis failure, but today both look like one.
6. **Cross-TU.** Tables, vtables and registries are usually defined in one TU and used in another. Static targets have no public symbol (X04).
7. **Signature laundering.** fptr casts and `void*` round trips hide the signature that really runs (F13, F14). C17 6.3.2.3p8 / 6.5.2.2p9 make such calls undefined unless the types are compatible.
8. **Reporting honesty.** `callbackMediation=Resolved*` is set for F07/F13/F20/F24/F27/F28 even though no evidence was transferred. Coverage must be measured on what was actually composed.

## Review round 2 additions (2026-11-25)

Measured on the same llvm-18 build. Fixture details are in [01-fact-catalog.md §F2](01-fact-catalog.md#f2-review-round-2-additions-2026-11-25); the reasoning is in [05-plan-review.md](05-plan-review.md).

| # | Pattern | Today | Root cause |
|---|---|---|---|
| **F36** | `if (a) cb=f; else cb=g; if (b) cb=h; cb(ctx)` | ⚠️ `long*` only | `TraverseIfStmt` (`:3074`) does not snapshot `functionPointerTargetSets`; `mergeFunctionPointerStates` (`:25352`) joins only the single-target maps, so a set that exists before the `if` is lost on the untouched path. The same code labels the untouched path `[always]` instead of `[!(b)]`. |
| F37 | nested `struct ops` inside two different outer records, set and called in different functions | ❌ Opaque | no L1 record slots yet. **Design risk:** keying slots by the innermost type (D7) would merge both outer records into one false multi-type set. |
| F42 | slot read through `(struct base *)` with layout-compatible derived records | ❌ Opaque | no L1 yet. **Design risk:** a type-keyed slot for `struct base` has no stores, so a naive L1 lookup would be confidently empty. |
| **F43** `run` | global with an initializer, overwritten in another function by an external call | ⚠️ `int*` only | same as F35; the escaped/unknown store must make the slot ⊤ |
| F43 `run_local` | the same overwrite right before the call | ✅ Opaque | L0 erases the binding. **Design risk:** L0 cannot tell "no info" from "killed by unknown". An L1 fallback behind L0 would bring the initializer back. |
| F38–F41 | strong update between calls, HOF reassign, nested/multi-slot locals, `(*s.fn)(x)` | ✅ | controls that M2/M3 must not break |
| F44 *(round 3)* | `if (x) { cb = g; return cb(a); } return cb(b);` | ❌ imprecise: `b` gets `{f, g}` | early exits are not modelled; the then-state is joined after the `if` |

**Totals after round 2:** 56 fixtures, 65 subjects, baseline drift 0. Subjects with a **wrong** answer today: F09, F10, F35, X11, X12, plus **F36** and **F43:run** (7).

### Additional challenges

9. **State encoding.** L0 keeps a single-target map and a target-set map side by side; branch merges read the first one only (F36). Every new join (switch, loop) must work on sets and treat "absent" as one of three states: *no info*, *killed by an unknown store*, *known set*.
10. **Slot identity for nested and cast records.** Object-insensitive record slots are only sound and precise if the key is the outermost record visible in the access path (F37). Reads through a cast to another record type must union every slot that the cast can alias (F42).
11. **Deterministic output.** Target sets are `std::set<const FunctionDecl*>`, which orders by address. New sets must order by a stable key, or the guard lists will reorder between runs.
12. **Gate integrity.** Any stray `examples/fptr-*.c` used to abort the checker (fixed in M0.5). The fptr gate now runs in ctest under the label `fptr`.

## Round 4 additions (2026-11-25): CFG evidence

Measured on fixtures F45–F56 and X14 ([01 round 4](01-fact-catalog.md)). More wrong answers that come from having no CFG:
- F46 `while`, F47 switch fallthrough, F50 backward goto and F51 dispatch loop each lose a reachable target.
- F45 is imprecise.

Function-type casts need labels rather than a single flag (D29). For cast history, what matters is **dispatch correlation**, not guards alone (D30).

**Totals after round 4:** 70 fixtures, 80 subjects, drift 0.

**Wrong answers today:** F09, F10, F35, F36, F43:run, F46:run_while, F47, F50, F51, X11 and X12 (11 subjects).

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
| F27 | recursion only through a table | ❌ | indirect edges are absent from the SCC |
| F28 | fptr via `va_arg` | ❌ | not a named formal |
| F29 | `if (cb == use_int) cb(x)` | ❌ | no equality refinement |
| F31 | callback formal in a generic helper | ✅ (parametric) | must stay parametric, not be counted as a failure |

**Totals:** 48 fixtures, 50 subjects. 6 already have the required category and types: F01, F02, F03 (by traversal order only), F14 (without the mismatch flag), F25 and F31. 5 subjects get a **wrong** answer today: F09, F10, F35, X11 and X12. The full matrix is in [evidence/evidence-table.md](evidence/evidence-table.md).

## The underlying challenges

1. **Target identity is a storage (points-to) problem.** fptrs live in globals, arrays, struct slots, unions, return values and out-parameters. These outlive one function body, and today's state does not.
2. **Flow-sensitivity without a CFG.** The engine walks the AST once. For data-pointer origins the approximation is tolerable. For fptr state it is **unsound**, because a strong update in AST order deletes a reachable target (F09, F10, F35).
3. **The caller ↔ callee link is the evidence.** An unresolved call does not merely lose precision. It removes all of the callee's operations from the caller's history, so `ctx` *looks* like pure transport.
4. **Pairing.** A callback and its `void*` context are stored together and reunited later. Tracking each half separately crosses the pairs (F19), so the pairing must be preserved.
5. **External callers.** libc/POSIX functions invoke the callbacks themselves (`pthread_create`, `qsort`), and `dlsym` targets are not in the project. Neither is an analysis failure, but today both look like one.
6. **Cross-TU.** Tables, vtables and registries are usually defined in one TU and used in another. Static targets have no public symbol (X04).
7. **Signature laundering.** fptr casts and `void*` round trips hide the signature that really runs (F13, F14). C17 6.3.2.3p8 / 6.5.2.2p9 make such calls undefined unless the types are compatible.
8. **Reporting honesty.** `callbackMediation=Resolved*` is set for F07/F13/F20/F24/F27/F28 even though no evidence was transferred. Coverage must be measured on what was actually composed.

# 01 — fptr fact catalog

Each **fact** is one thing the analyzer must know or do so that a `void*` reaching a function pointer can be classified. Each fixture shows exactly one fact. Single-TU fixtures are `examples/fptr-NN-*.c` (IDs `Fnn`). Cross-TU fixtures are `test_projects/projects/p1NN_fptr_xtu_*` (IDs `Xnn`). The machine-readable contract is the `@fptr-fact / @subject / @baseline / @expected` comment block in each fixture. `scripts/check_fptr_evidence.py` enforces it.

**Capability** column: the layer or feature that must exist for the fact to hold. See [02-design.md](02-design.md).

| Code | Meaning |
|---|---|
| L0 | flow-sensitive local fptr state |
| L1 | flow-insensitive storage facts (TU- or project-wide) |
| L2 | signature fallback |
| LIB | library callback model |
| CCB | callback–context binding |
| RET | return / out-parameter target summaries |
| CF | switch join / loop widening |
| FACET | the `indirectCallResolution` report facet |

**Citation sources:**
- `bench:` paths are in this repository under `test_projects/` and were checked with `grep` at the line given.
- `ext:` items are well-known public code, cited by file or API name. Line numbers are not given because they drift.
- `idiom:` marks a common C pattern with no instance pinned in this repository's benchmarks.

## A. Where the target comes from (resolution)

| ID | Fact | Fixture | Capability | Real-world citation |
|---|---|---|---|---|
| F01 | local fptr var initialized with a designator | `fptr-01-local-var-known-target.c` | L0 (exists) + FACET | ubiquitous |
| F02 | null-guarded call `if (cb) cb(x)`; the null arm is not a target | `fptr-02-null-guarded-call.c` | L0 (exists) | bench: `icecast-2.4.4/src/format.h:54` optional `apply_settings` hook |
| F03 | global with constant initializer, no other stores | `fptr-03-global-initializer.c` | L1 (works today only by traversal order) | bench: `parson/parson.c:62` `static JSON_Malloc_Function parson_malloc = malloc;` |
| F34 | global defined after its use | `fptr-34-global-defined-after-use.c` | L1 (order independence) | any TU with forward `extern` declarations |
| F35 | global initialized, then reassigned in another function (**unsound today**) | `fptr-35-global-reassigned-elsewhere-unsound.c` | L1 | bench: `parson/parson.c:62` + `:2052` |
| F04 | global set by a setter, called elsewhere | `fptr-04-global-registry-assigned-elsewhere.c` | L1 | bench: `parson/parson.c:2052` `parson_malloc = malloc_fun;` |
| F32 | global set by `__attribute__((constructor))` | `fptr-32-constructor-attribute-registration.c` | L1 | ext: GCC constructor attribute; plugin self-registration |
| F05 | const dispatch table `tbl[i](x)` | `fptr-05-const-dispatch-table.c` | L1 (array slot) | bench: `lua-5.4.1/src/linit.c:42` `static const luaL_Reg loadedlibs[]` |
| F06 | mutable table filled in `init()` | `fptr-06-runtime-filled-dispatch-table.c` | L1 | bench: `bench-genc2rust/lil/main.c:871` `lil->callback[cb] = proc;`, read+cast at `:501` |
| F07 | `{name, fn}` table searched by a loop | `fptr-07-named-command-table.c` | L1 (struct-array slot) | bench: `lua-5.4.1/src/lauxlib.c:904-913` `luaL_setfuncs` walks `l->func` |
| F33 | heterogeneous table: selector-guarded alternatives | `fptr-33-heterogeneous-table-selector-guard.c` | L1 + D8 guard policy | idiom: printers/visitors keyed by a type tag (no bench instance pinned) |
| F08 | `c ? f : g` | `fptr-08-ternary-select.c` | L0 (expression form) | ubiquitous |
| F11 | factory returns an fptr | `fptr-11-factory-return.c` | RET | bench: `ImageMagick-7.0.11-6/MagickCore/magick.c:223` `DecodeImageHandler *GetImageDecoder(const MagickInfo *)` |
| F12 | out-param `handler_t *out` | `fptr-12-out-param-pointer-to-fptr.c` | RET | idiom: lookup APIs returning a handler via out-param (no bench instance pinned) |
| F13 | fptr → `void*` → fptr | `fptr-13-fptr-through-void-pointer.c` | L0 (binding through `void*`) | bench: `lua-5.4.1/src/loadlib.c:141` `cast_func(dlsym(lib, sym))` |
| F14 | call through an incompatible signature | `fptr-14-incompatible-signature-cast.c` | FACET `signatureMismatch` | bench: `thttpd-2.29/extras/htpasswd.c:159` `signal(SIGINT,(void (*)(int))interrupted)`; ext: GLib `G_CALLBACK()` |
| F25 | fptr in a union member | `fptr-25-fptr-in-union.c` | L0 (exists; keep) | bench: `thttpd-2.29/timers.h:41` `ClientData` union |
| F26 | ops struct copied by `memcpy` | `fptr-26-ops-struct-memcpy.c` | L0/L1 (memcpy = struct copy) | idiom: cloning a default ops table before overriding slots (no bench instance pinned) |
| F28 | fptr passed via `...`/`va_arg` | `fptr-28-variadic-fptr-argument.c` | L1 (variadic actuals) | ext: libcurl `curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, cb)` |
| F29 | `if (cb == f) cb(x)` narrows the target | `fptr-29-fptr-equality-refinement.c` | L0 (guard refinement) | ext: `if (handler == SIG_DFL)`, default-allocator fast paths |
| F31 | callback formal in a generic helper stays *parametric* | `fptr-31-formal-callback-parametric.c` | FACET (`parametric`) | bench: `examples/callback-provider-vs-dependent.c` |

## B. Control flow around fptr state

| ID | Fact | Fixture | Capability | Citation |
|---|---|---|---|---|
| F09 | targets assigned in `switch` arms (**unsound today**) | `fptr-09-switch-select-unsound.c` | CF switch join | ext: opcode `switch` assigning a handler |
| F10 | target reassigned on a loop back edge (**unsound today**) | `fptr-10-loop-backedge-unsound.c` | CF loop widening | ext: retry loops switching to a fallback handler |
| F17 | FSM: the state function returns the next state | `fptr-17-fsm-next-state.c` | RET + CF + L1 | ext: common tokenizer/protocol idiom (no bench instance pinned) |
| F27 | recursion closed only through an indirect call | `fptr-27-recursion-through-dispatch-table.c` | indirect call-graph edges in the SCC | bench: `lua-5.4.1/src/ldo.c:481` `n = (*f)(L);` re-enters the interpreter |

## C. Objects, vtables, OOP

| ID | Fact | Fixture | Capability | Citation |
|---|---|---|---|---|
| F15 | vtable: `o->vt->m(o->data)`; ops installed elsewhere | `fptr-15-vtable-struct-type-slot.c` | L1 struct-type slot | bench: `libarchive-3.4.3/libarchive/archive_private.h:63` `struct archive_vtable`, called at `archive_virtual.c:36`; ext: Linux `struct file_operations` (`include/linux/fs.h`) |
| F30 | fn field stored directly in a heap object | `fptr-30-heap-object-fptr-field.c` | L1 struct-type slot | bench: `bench-genc2rust/lmdb/main.c:3938` `->md_cmp = & mdb_cmp_long;` |
| F16 | OOP: base embedded first, method takes `void *self` and downcasts | `fptr-16-oop-void-self-downcast.c` | L1 struct-type slot | bench: `icecast-2.4.4/src/format.h:57` `void *_state;`, `format_ogg.c:279` `ogg_state_t *ogg_info = source->format->_state;`, install `format_mp3.c:87`; ext: SQLite `sqlite3_file{const sqlite3_io_methods *pMethods}` extended by `unixFile`; CPython `PyObject_HEAD` + `PyTypeObject` `tp_*` slots |

## D. Data flowing through indirect calls

| ID | Fact | Fixture | Capability | Citation |
|---|---|---|---|---|
| F18 | callback + `void*` context registered, fired later | `fptr-18-callback-context-registration.c` | CCB | bench: `bench-genc2rust/c-thread-pool/main.c:345` `thpool_add_work(pool, function_p, arg_p)`, fired `:530`; `thttpd-2.29/timers.h:72` `tmr_create(..., TimerProc*, ClientData, ...)`, fired `timers.c:266` |
| F19 | two registrations, different types; must not cross | `fptr-19-callback-context-pair-correlation.c` | CCB (per registration site) | bench: thttpd timer list mixes ClientData kinds (`timers.c:266`) |
| F20 | `struct job {fn,arg}` passed to an executor | `fptr-20-job-struct-fn-arg.c` | struct-relative higher-order summary | bench: `bench-genc2rust/bzip2/main.c:112` `bzalloc`, call `:7857` `(*(strm->bzalloc))(strm->opaque, ...)` |
| F24 | indirect call returns (an alias of) its argument | `fptr-24-indirect-call-return-alias.c` | return-alias continuation for indirect calls | bench: `lua-5.4.1/src/lmem.c:35` `return (*g->frealloc)(g->ud, block, os, ns);` |

## E. External boundaries

| ID | Fact | Fixture | Capability | Citation |
|---|---|---|---|---|
| F21 | `pthread_create` forwards `arg` to `start_routine` | `fptr-21-library-callback-pthread-create.c` | LIB | bench: `bench-genc2rust/c-thread-pool/main.c:470` |
| F22 | `qsort` passes *element addresses* of `base` to `cmp` | `fptr-22-library-callback-qsort.c` | LIB (element relation) | bench: `thttpd-2.29/tdate_parse.c:114` |
| F23 | `dlsym` target is outside the project → external, not unresolved | `fptr-23-dlsym-plugin.c` | LIB + FACET | bench: `lua-5.4.1/src/loadlib.c:141`; `ImageMagick-7.0.11-6/MagickCore/module.c:1302`; ext: POSIX `dlsym` rationale on `void*` ↔ function-pointer conversion |

## F. Cross-TU twins

| ID | Fact (twin of) | Fixture |
|---|---|---|
| X01 | extern dispatch table defined in another TU (F05) | `p132_fptr_xtu_extern_dispatch_table` |
| X02 | global hook set through another TU's setter; slot target arrives via the caller binding (F04) | `p133_fptr_xtu_global_registry_via_setter` |
| X03 | vtable instance + ctor in b.c, virtual call in a.c (F15) | `p134_fptr_xtu_vtable_struct_type_slot` |
| X04 | factory returns a **static** target of another TU (F11) | `p135_fptr_xtu_factory_returns_static_target` |
| X05 | registry library in b.c, registration in a.c (F18) | `p136_fptr_xtu_callback_context_registration` |
| X06 | pair correlation across TUs (F19) | `p137_fptr_xtu_callback_pair_correlation` |
| X07 | FSM states in b.c, driver in a.c (F17) | `p138_fptr_xtu_fsm_states` |
| X08 | OOP class in b.c, dispatcher in a.c (F16) | `p139_fptr_xtu_oop_void_self_downcast` |
| X09 | thread entry in b.c, `pthread_create` in a.c (F21) | `p140_fptr_xtu_library_callback_pthread` |
| X10 | `dlsym` name equals a project symbol; still external (F23, D6) | `p141_fptr_xtu_dlsym_name_matches_project_symbol` |
| X11 | switch select with cross-TU targets, **unsound today** (F09) | `p142_fptr_xtu_switch_select_unsound` |
| X12 | loop back edge with cross-TU targets, **unsound today** (F10) | `p143_fptr_xtu_loop_backedge_unsound` |
| X13 | job executor in b.c (F20) | `p144_fptr_xtu_job_struct_executor` |

## F2. Review round 2 additions (2026-11-25)

These came from the owner's probes (originals kept byte-identical in [`examples/probes/`](../../examples/probes/README.md)) and from the plan review ([05-plan-review.md](05-plan-review.md)). "Control" means the category and types are already right today; only the `resolution` facet is missing, so M1 claims them.

| ID | Fact | Fixture | Layer / milestone | Today |
|---|---|---|---|---|
| F36 | one-armed `if` after a multi-target merge drops earlier targets (**unsound today**) | `fptr-36-one-armed-if-after-merge-unsound.c` | L0, M2 (seam S5) | ⚠️ `long*` only |
| F37 | fptr in a nested record shared by two outer records, installed and called in different functions | `fptr-37-nested-record-slot-cross-function.c` | L1 record slot, M3c, **D7′** | ❌ Opaque |
| F38 | local fptr strongly updated between two calls | `fptr-38-strong-update-between-calls.c` | L0 control, M1 | ✅ |
| F39 | fptr reassigned between two higher-order calls; `hof::data` stays parametric | `fptr-39-hof-reassign-between-calls.c` | L0 + `$k.$i` control, M1 | ✅ |
| F40 | nested ops record with two slots, two local instances | `fptr-40-nested-and-multi-slot-local.c` | L0 per-object overlay control, M1 | ✅ |
| F41 | explicit dereference call `(*s.fn)(ctx)` on sibling record types | `fptr-41-deref-call-through-field.c` | resolver normalisation control, M1 | ✅ |
| F42 | slot read through a cast to a layout-compatible base record | `fptr-42-base-prefix-cast-slot.c` | L1 soundness guard for D7′, M3c | ❌ Opaque (must become a sound set, or `unresolved`) |
| F43 | global killed by an unknown store, in the same function (`run_local`) or in another one (`run`) | `fptr-43-global-killed-by-unknown-store.c` | L0 tri-state + L1 ⊤, M3a | `run_local` ✅ Opaque; `run` ⚠️ `int*` only (same defect as F35) |

### Round 3 addition (2026-11-25)

| ID | Fact | Fixture | Layer / milestone | Today |
|---|---|---|---|---|
| F44 | a branch ending in `return` is still joined after the `if`, so an infeasible target reaches later calls | `fptr-44-early-return-merge-imprecise.c` | CF early exit, M2 if D23 is accepted | ❌ `UnguardedMultiTarget`, `double*,int*` (should be `int*`) |

### Round 4 additions (2026-11-25): CFG evidence, see [08](08-cfg-fptr-resolution.md)

| ID | Fact | Fixture | Milestone | Today (measured) |
|---|---|---|---|---|
| F45 | loop exits only via `break` after a reassignment | `fptr-45-loop-exit-only-via-break.c` | M2a | imprecise set (should be exact `g`) |
| F46 | do-while (≥1 iteration) vs while (0 iterations possible) | `fptr-46-do-while-vs-while.c` | M2a | `run_do` ✅; `run_while` ⚠️ exact `double*` |
| F47 | switch fallthrough into the call | `fptr-47-switch-fallthrough.c` | M2b | ⚠️ exact `double*` |
| F48 | `&&` guard on the assignment | `fptr-48-short-circuit-guard.c` | M2b | set ✅ by accident (no conjunction guard) |
| F49 | forward goto cleanup | `fptr-49-goto-cleanup.c` | M2c (control) | set ✅ by accident |
| F50 | backward goto retry loop | `fptr-50-goto-retry-loop.c` | M2c | ⚠️ exact `int*` |
| F51 | dispatch loop with switch, continue, break, return | `fptr-51-dispatch-loop-switch.c` | M2c | ⚠️ exact `double*` |
| F52 | recursion through a local fptr variable | `fptr-52-recursion-through-local-fptr.c` | M4 | types right, `unresolved`, no SCC edge |
| F53 | fn-pointer transport round trip (`void(*)(void)`) | `fptr-53-fnptr-transport-roundtrip.c` | M1 (control) | ✅ |
| F54 | table dispatch selected by the object (`tbl[o->kind]`) | `fptr-54-object-correlated-table.c` | M3b | ❌ unresolved |
| F55 | table dispatch with an unrelated selector | `fptr-55-unrelated-table-selector.c` | M3b | ❌ unresolved |
| F56 | arity-mismatching fn-type cast | `fptr-56-fnptr-arity-mismatch.c` | M2b | ❌ not flagged |
| X14 | a.c `lookup` returns via switch; b.c calls `lookup(i)(x)` in a loop | `p145_fptr_xtu_switch_return_in_loop` | M8c | ❌ unresolved |

## G. Documented, not fixtured (out of scope or no void* impact)

| ID | Pattern | Why not fixtured | Citation |
|---|---|---|---|
| N1 | `signal()`, `atexit()`, `pthread_once()` | the callback receives no data pointer, so no void* evidence flows. Still recorded as LIB entries so they are not reported as unresolved | bench: `thttpd-2.29/extras/htpasswd.c:159` |
| N2 | computed goto label tables (`goto *disptab[op]`) | not function pointers; label addresses (GNU extension). The existing feature report already marks indirect goto unsupported | bench: `lua-5.4.1/src/ljumptab.h:12,19` |
| N3 | linker-section registration tables (`__attribute__((section))`, Linux initcalls) | targets come from link order, not source; treated as L2 signature candidates only | ext: Linux `include/linux/init.h` |
| N4 | `pthread_key_create` destructor / `tss_create` | the destructor's argument comes from a later `pthread_setspecific`; it needs a key-slot model. Deferred (LIB-2) | ext: POSIX |
| N5 | GLib/libuv style `user_data` APIs (`g_signal_connect`, `g_list_foreach`, `uv_queue_work`) | same shape as F18/F21. Covered by the LIB table format, but only libc/POSIX entries ship first | ext: GLib, libuv |
| N6 | `qsort_r`, `bsearch`, `tsearch/twalk`, `lfind`, `nftw`, `scandir`, `dl_iterate_phdr`, `on_exit`, `thrd_create` | additional LIB rows, same mechanism as F21/F22 | ext: glibc/POSIX/C11 |
| N7 | weak symbols / `LD_PRELOAD` interposition | the target is decided at link or load time. Reported as external | ext: ELF symbol interposition |
| N8 | K&R unprototyped `void (*)()` | signature unknown; L2 matching must treat it as a wildcard | ext: legacy C (e.g. older thttpd sources) |
| N9 | JIT / generated code, Apple blocks, C++ | out of scope for a C AST analyzer | — |
| N10 | irreducible loops (goto into a loop body) | supported by construction in the CFG solver (DFS retreating edges); fixture once a bench instance is pinned | ext: hand-written state machines |

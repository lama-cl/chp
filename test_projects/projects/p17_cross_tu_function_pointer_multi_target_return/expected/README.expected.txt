C3d regression: guarded multi-target function-pointer return composition.

Project shape:
  a.c defines f.  f chooses fp = g1 or fp = g2 under cond and returns fp(x).
  b.c defines g1 and g2.  Both return aliases of their first parameter after
  interpreting it through different pointer types.

Expected after the standard multi-TU workflow:

  Phase 1: summarize b.c and a.c.
  Phase 2: build project-summary-index.json.
  Phase 3: rerun a.c in import mode.

The import-mode output for a.c should show:

  Function summaries:
    f($0 : void * => ⟨[cond] g1⌜int *⌝g1, [!(cond)] g2⌜char *⌝g2⟩, $1 : int)
      -> void * [⟨[cond] g1⌜$0 => int * => void *⌝g1,
                  [!(cond)] g2⌜$0 => char * => void *⌝g2⟩]

Exact rendering may vary slightly depending on return-summary formatting, but
these semantic facts should hold:

  1. The parameter history for f::$0 remains branch-shaped, not parallel-shaped.
  2. The return summary is also branch-shaped because the function-pointer
     target is selected by the same cond / !(cond) branch split.
  3. The Cross-TU function-pointer target-set lookup reports returnImport=composed
     or returnImport=partial-composed and returnComposed.
  4. The JSON field crossTUFunctionPointerTargetSetLookups contains:
       returnComposed: true
       returnImportStatus: "composed" or "partial-composed"
       returnComposedTargets containing g1 and g2

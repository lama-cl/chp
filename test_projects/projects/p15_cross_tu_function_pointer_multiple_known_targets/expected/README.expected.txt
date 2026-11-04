Expected C3c-3 behavior for p15_cross_tu_function_pointer_multiple_known_targets.

Shape:
  if (cond)
    fp = g1;
  else
    fp = g2;
  fp(x);

Expected effects:
  - Cross-TU function-pointer summary lookup may remain <none> because there is
    no single known target.
  - Cross-TU function-pointer target-set lookup should report composition, not
    reporting-only behavior.  The expected status is one of:
      [multi-target-composed]
      [multi-target-partial-composed]
    depending on whether all candidate summaries are safely resolved.
  - For this regression, when the summary index contains b.c summaries for both
    g1 and g2, the target-set record should show:
      candidates: g1, g2
      C2-resolved: g1, g2
      composed targets: g1, g2
      [paramImport=composed]
      [pointerUseImport=composed]
  - Imported parameter summaries from g1/g2 should be projected onto x with the
    function-pointer target-selection guards preserved.  The expected readable
    entity history should have branch-join shape, not parallel-cast shape:
      f::x : void * => ⟨[cond] g1⌜int *⌝g1, [!(cond)] g2⌜char *⌝g2⟩
    The exact spacing may vary, but the outer notation should be ⟨ ... ⟩ rather
    than ⟨| ... |⟩ for this branch-selected target set.
  - The function-designator decays for g1 and g2 should be excluded from the
    ordinary Direct cast histories section and reported under Function pointer
    flows, for example:
      g1 : void (void *) => ⟨[cond] void (*)(void *)⟩    [function-designator decay]
      g2 : void (void *) => ⟨[!(cond)] void (*)(void *)⟩    [function-designator decay]
  - Imported pointer-use summaries from both g1 and g2 should transfer to x;
    those transferred uses should also carry the corresponding target-selection
    guard when shown in pointer-use summaries / JSON.
  - If neither target resolves in the summary index, the record should fall
    back to [multi-target-unresolved] and no multi-target import is expected.

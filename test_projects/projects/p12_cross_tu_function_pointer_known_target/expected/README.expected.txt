Expected C3b-4 behavior for p12_cross_tu_function_pointer_known_target.

This project exercises a known function-pointer target:

  void (*fp)(void *) = g;
  fp(x);

C1/C2:
  b.c exports a summary for g(void *).

C3 on a.c:
  The call fp(x) is an indirect call syntactically, but the flow-sensitive
  functionPointerTargets state knows fp -> g.  The C3b-4 pass should now import:
    - g's parameter cast summaries onto x
    - g's parameter pointer-use summaries onto x
    - g's return summary when the indirect-call result is returned directly or
      propagated through a local/field alias

Expected effects:
  - Cross-TU function-pointer summary lookup should report:
      [found] [composed] [paramImport=composed]
      [pointerUseImport=composed]

  - For a direct return form like:
      return fp(x);
    the lookup should also report:
      [returnImport=composed] [returnComposed]

  - For a local alias form:
      void *r = fp(x); return r;
    the lookup may show:
      [returnImport=call-result-origin-composed]
    because the imported return alias is recovered through originInfoForExpr.

  - The caller's return summary should preserve a connection to $0 and the
    imported call wrapper g⌜...⌝g, not collapse to only [void *].

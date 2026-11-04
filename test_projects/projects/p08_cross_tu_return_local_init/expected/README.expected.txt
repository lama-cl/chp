Expected C3a behavior:
- The imported call-result origin should flow through local r.
- f's return should still alias $0 and include imported g return behavior.
- Cross-TU lookup should include:
    [returnImport=call-result-origin-composed]
  when originInfoForExpr(g(x)) attaches g's return suffix to r's origin.

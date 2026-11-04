Purpose:
  Field-sensitive return propagation through a known function-pointer target.

Key code shape:
  void *(*fp)(void *) = g;
  b.p = fp(x);
  return b.p;

Expected C3b-4 effects on a.c:
  - Cross-TU function-pointer summary lookup reports:
      [found] [composed] [paramImport=composed]
    and usually:
      [returnImport=call-result-origin-composed]
    or
      [returnImport=composed] [returnComposed]
    depending on whether the return path is recorded directly at return fp(x)
    or recovered through originInfoForExpr on the field return.

  - Function summary for f should not collapse to:
      -> void * [void *]

Expected summary shape:
  f($0 : void * => g⌜char *⌝g) -> void * [... $0 ... g⌜...⌝g ...]
      [ret aliases $0]

Acceptable equivalent renderings:
  - [$0 => g⌜char * => void *⌝g]
  - [g⌜$0 => char * => void *⌝g]

The critical property is that the return summary preserves a connection to the
parameter $0 through the imported function-pointer target g.

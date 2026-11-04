Purpose:
  Direct return through a known function-pointer target.

Key code shape:
  void *(*fp)(void *) = g;
  return fp(x);

Expected C3b-4 effects on a.c:
  - Cross-TU function-pointer summary lookup reports:
      [found] [composed] [paramImport=composed]
      [returnImport=composed] [returnComposed]

  - Function summary for f preserves both:
      parameter-side imported cast history on $0
      return-side imported call wrapper from g

Expected summary shape:
  f($0 : void * => g⌜char *⌝g) -> void * [g⌜$0 => char * => void *⌝g]
      [ret aliases $0]

Acceptable equivalent renderings:
  - The exact call-wrap placement may vary slightly.
  - The critical requirement is that the return summary is not only [void *];
    it should preserve both $0 and the imported g⌜...⌝g path.

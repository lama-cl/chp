Expected C3a behavior:
- a.c resolves g through the C2 summary index.
- f's parameter receives g's parameter-side summary:
    f::x : void * => g⌜char *⌝g
- f's return summary imports g's return:
    f($0 : void * => g⌜char *⌝g) -> void * [g⌜$0 => char * => void *⌝g]
- Cross-TU lookup should include:
    [paramImport=composed]
    [returnComposed]
    [returnImport=composed]

C6g4w1 regression target: source-level pointer and cast prevalence metrics.

Run the analyzer with:
  --prevalence-metrics-json <file>
  --prevalence-metrics-project-root <absolute path to this project>
  --prevalence-include-return-types

Required raw facts:
  1. Pointer declaration rows distinguish global, local, parameter, field, and
     opt-in return declarations.
  2. `void *` declarations include direct outer `void *` types only; no `void **`
     declaration is silently classified as a void-pointer declaration.
  3. Function-pointer declarations include `void (*)(int)` globals, fields, and
     parameters. Pointer fields remain a subset of all pointer declarations.
  4. Explicit cast rows include text -> void*, void* -> char*, char* -> unsigned
     char*, and unsigned char* -> int*.
  5. The text -> void* cast increments To void*; void* -> char* increments From
     void*. Every explicit cast whose source or destination is char*, signed
     char*, or unsigned char* increments Byte casts.
  6. The assignment `char *implicit_from_void = input` is represented as an
     implicit CastExpr metric independently of --include-implicit.

Scope boundary:
  - The prevalence sidecar is observational and does not alter cast-history,
    pointer-use, eager-transfer, named-call, or fixpoint behavior.

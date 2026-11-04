C6g4x1a regression target: diagnostic containment and bounded rendering

Run with --analysis-progress.  The local pointer-use attachment diagnostic is
strictly observational.  It may retain bounded text or disable itself after a
std::length_error, std::bad_alloc, or other diagnostic-only exception.

Required properties:
  1. A diagnostic rendering failure must not abort AST traversal.
  2. The progress line must report:
       local-pointer-use-attachment-diagnostic-state=disabled
       local-pointer-use-attachment-diagnostic-failure-kind=...
       local-pointer-use-attachment-diagnostic-failure-stage=...
     when containment is exercised.
  3. Pointer-use membership, cycle suppression, summary solving, emitted
     histories, and prevalence sidecars remain unaffected.
  4. Normal records retain no more than 64 diagnostic attachment tokens and
     512 characters per rendered diagnostic field.

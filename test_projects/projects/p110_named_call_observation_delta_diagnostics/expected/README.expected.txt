C6g4w2 regression target: named-call parameter observation delta diagnostics

Run with --analysis-progress.  The completed named-call round that changes a
function parameter summary must report up to four bounded observation deltas.
A delta reports:
  1. direct-param-union or guarded-field-function-pointer update path;
  2. active caller -> callee edge and the target function parameter;
  3. optional selection guard;
  4. stable before/after FNV-1a fingerprints and full-length counts;
  5. bounded structural shapes and bounded before/after history text.

Required properties:
  1. The diagnostic is observational and must not alter summary membership,
     convergence, or emitted history text/JSON.
  2. Detailed samples appear only in named-call-round-complete records, not on
     every periodic progress line.
  3. Retention is bounded at four samples per round.  Extra samples increment
     named-call-round-observation-delta-samples-truncated.
  4. An allocation/rendering failure disables only the delta diagnostic and
     leaves named-call analysis running.

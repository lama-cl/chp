C6g4p regression target: acyclic same-TU calls do not widen caller guards.

Expected for caller::$0:
  - the two calls to leaf preserve their distinct mode / !mode full guards;
  - no C6g4p recursive-guard projection is applied to either acyclic edge.

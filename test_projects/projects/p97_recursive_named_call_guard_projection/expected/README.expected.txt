C6g4p regression target: recursive same-TU guard projection.

Call graph:
  f -> g (under mode)
  f -> g (under !mode)
  g -> f

Expected:
  - the unguarded dereference/write in g remains an unguarded leaf operation
    in recursive pointer-use solver membership;
  - distinct caller contexts mode and !mode do not create two recursive solver
    facts for that one leaf operation;
  - --analysis-progress reports a nonzero
    named-call-recursive-guard-projected count when the transfer is exercised;
  - recursive evidence remains present for f::$0 and g::$0.

C6g4y2 AST-local pre-materialization ptrarith-run regression

For a source-local AST leaf whose attachment is a pure repeated ptrarith(T)
sequence, C6g4y2 forms the projected membership key before normalizeObs().
If the key already has a representative, the candidate is rejected without
materializing or rendering the long attachment path.

The first representative retains one ptrarith(T) atom. Generic and specific
dereference precedence keeps its existing exact handling.

With --analysis-progress, repeated candidates increment
ast-local-projected-pre-materialization-suppressed and
ast-local-projected-ptrarith-run-components-subsumed.

C4a dependency-edge expectations for demo4.c
===========================================

This project exercises the per-TU summaryDependencyEdges JSON field.

Important source facts:
  - main calls ho(pfunc, pv) twice.
  - ho calls its callback parameter pf(pcho).
  - pfunc resolves to idv for the first ho call and fret for the second ho call.

Expected summaryDependencyEdges in demo4.cast-summary.json include at least:

  main -> ho
    kind: direct-call

  ho -> $0
    kind: higher-order-param-call
    calleeParamIndex: 0
    calleeExpr: pf

  main -> idv
    kind: function-pointer-actual
    via: ho.$0

  main -> fret
    kind: function-pointer-actual
    via: ho.$0

The exact file paths and source locations are environment-specific.
C4a does not compute SCCs yet; build_summary_index.py only merges the per-TU
edges into project-summary-index.json and reports dependency-edge counts.

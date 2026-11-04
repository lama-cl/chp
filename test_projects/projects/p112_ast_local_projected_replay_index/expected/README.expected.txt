C6g4y1 AST-local projected replay-index regression

AST-local records remain keyed by C6g4y's immutable leaf projection.
C6g4y1 stores a per-entity projected-key index, so repeated local candidates
reach their representative without scanning every pointer-use record.

The membership renderer walks the existing ObsHist directly and collapses
adjacent same-type ptrarith atoms without constructing normalized ObsHist nodes.
Full route/guard witnesses are inspected only within a bounded budget; later
replays are semantically deduplicated without attachment or guard rendering.

With --analysis-progress, repeated local candidates report
last-pointer-use=deduplicated-ast-local-projected-indexed and increment
ast-local-projected-membership-index-hits.

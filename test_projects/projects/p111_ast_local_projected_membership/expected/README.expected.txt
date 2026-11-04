C6g4y AST-local projected membership regression

The pointer-arithmetic use and call argument are source-local AST leaves.
Their membership retains entity, normalized attachment shape, use type/kind,
immutable operation site, lexical operation-local guard, and leaf detail.

Repeated reaching guards, mutable reporting span, route provenance, and an
adjacent repeated same-type ptrarith run are projected out of local identity.
They remain bounded witnesses on the retained representative.

With --analysis-progress, a repeated route is reported as
last-pointer-use=deduplicated-ast-local-projected rather than inserted.

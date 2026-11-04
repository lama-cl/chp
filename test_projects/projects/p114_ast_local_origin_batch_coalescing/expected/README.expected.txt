C6g4y3 AST-local origin-batch coalescing regression

For a source-local non-dereference leaf, origin alternatives that differ only
in reaching/route guard are coalesced before recordPointerUse().  The batch key
uses the same projected local membership dimensions as C6g4y/C6g4y2:
source entity, normalized attachment, pointer-use shape, immutable operation
site, lexical operation-local guard, and semantic detail.

Generic/specific dereference precedence remains exact and is not coalesced by
this path.  Direct DeclRefExpr leaves read originState by const view instead of
copying a potentially large OriginInfo vector.

With --analysis-progress, C6g4y3 reports:
  ast-local-origin-batch-raw-alternatives
  ast-local-origin-batch-unique-projected-alternatives
  ast-local-origin-batch-coalesced-alternatives
  ast-local-origin-batch-direct-state-views
  ast-local-origin-batch-snapshot-views

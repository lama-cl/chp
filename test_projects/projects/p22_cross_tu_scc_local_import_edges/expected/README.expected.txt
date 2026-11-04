C4f regression: cross-TU recursive SCC with a local relay edge.

Project shape:
  f -> relay -> g -> f

Expected behavior after building the summary index and rerunning in import mode:
  - the index reports one recursive concrete-function SCC containing f, relay,
    and g;
  - the Cross-TU SCC parameter-summary fixpoint considers both imported and
    local same-SCC named-call edges;
  - g::pi's continuation through imported f is propagated back to g::x;
  - f::pc's continuation through local relay is propagated back to f::x;
  - relay is no longer limited to only the first imported int* step.

The key C4f expectation is that the imported direct-parameter edge:

  relay(x) -> g(x)

is treated as an SCC transfer edge.  Since x is relay's own formal parameter,
relay must not prepend its own current/direct parameter summary before applying
imported g.$0; otherwise the transfer prematurely returns to the initial int*
and truncates away the f/char* continuation.

Representative improved shape:
  relay($0 : void * => int * => void * => char *)
  f($0     : void * => char * => void * => int *)
  g($0     : void * => int * => void * => char *)

Exact wrapper rendering may differ depending on report-domain formatting, but
relay should contain more than just int*.

This test is intentionally multi-TU, so it belongs under test_projects/projects.

C6g4z4 shared callback-binding identity regression

`invoke` forwards both pointer parameters to its function-valued third formal.
At `top`, that formal is uniquely bound to `compare_identity`, whose two
parameters have no cast summaries.  The binding is cast-history identity.

Expected generic summaries:
  invoke.$0 : const void * => $2.$0
  invoke.$1 : const void * => $2.$1

Expected specialized caller summary:
  top.$0 : const void *

No semantic top.$0 history may contain `$2.$0`, `$2.$1`,
`<unknown:compare_identity.$0-unsummarized>`, or
`<unknown:compare_identity.$1-unsummarized>`.

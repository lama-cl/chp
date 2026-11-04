C6g4z3 edge-snapshot higher-order identity regression

`bridge` is generic and applies its function-valued parameter to `value` and
`bytes`.  `call_bridge` binds that parameter uniquely to `compare_identity`,
whose two parameters have no cast history.

Required behavior:

* `bridge` may retain its generic summaries ending in `$0.$0` / `$0.$1`.
* `call_bridge.$0` must not contain `$0.$0` or `$0.$1` leaked from `bridge`.
* The only non-identity propagated path is:

    const char *

The target binding must be read from the recorded direct NamedCallEdge; it must
not be rediscovered from live AST state during a later fixpoint round.

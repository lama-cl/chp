C6g4z if-condition direct-call traversal regression

The custom TraverseIfStmt models branch flow itself.  It must nevertheless
traverse, exactly once and before branch-state snapshots:
  1. the optional if initializer,
  2. the optional condition-variable declaration, and
  3. the condition expression.

Required direct same-TU named-call edges:
  wrapper -> check_bytes
  api -> wrapper

Both edges originate from CallExpr nodes inside equality comparisons used as
if conditions.  The calls execute under the enclosing reaching guard; they
must not be assigned the synthetic if-then or if-else guard, because condition
evaluation precedes branch selection.

After the named-call fixed point, the pointer-use/cast history caused by
check_bytes reading through its const void * parameter must be reachable from
wrapper.$0 and api.$0.  This fixture guards against losing parameter-to-
argument propagation merely because the call appears in an if condition.

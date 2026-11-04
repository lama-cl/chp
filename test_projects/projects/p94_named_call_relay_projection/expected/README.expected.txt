C6g4n regression target: named-call relay projection.

Requirements:
  - entry.$0 receives one fixed-point pointer-use fact for the dereference in
    consume(), even though it reaches that leaf through both relay_a.$0 and
    relay_b.$0.

  - The stored solver record retains the leaf use kind, pointer/access type,
    attachment, guard, and the original consume() source location.  Its detail
    is the leaf operation rather than a recursively growing list of
    `recursive via f.$i` / `via f.$i` relay clauses.

  - The alternate relay clauses are retained only in the bounded named-call
    relay-witness side table (maximum four per projected operation), and do
    not create additional fixed-point facts.

  - distinct_leaf_locations preserves the two separate use sites in
    consume_twice(): leaf source location remains part of projection identity.

Non-goals:
  - Direct/local pointer-use records are unchanged.
  - Data-field and function-pointer relay detail is not stripped merely because
    it begins with `via`; only exact named-formal clauses ending in `.$N` are
    projected away.

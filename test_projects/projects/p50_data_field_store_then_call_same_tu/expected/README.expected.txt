Expected C6d behavior:
  consume.$0 has local pointer operations: init-rhs to int* and deref-readwrite as int*.

  The call consume(box.payload) should transfer consume.$0 pointer-use facts to:
    run::box.payload
  and to the origin currently stored in that field:
    run::x

  The propagated run::x rows should preserve data-field provenance:
    origin=data-field, field=run::box.payload
  in addition to the same-TU callee provenance where applicable.

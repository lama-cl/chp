Expected C5b behavior:
  Field-sensitive function-pointer targets contains guarded assignments:
    run::ops.process -> handle_int  [guard=tag]
    run::ops.process -> handle_char [guard=!(tag)]
  Cross-TU function-pointer target-set lookup for a.c contains:
    calleeExpr = ops.process
    targets include handle_int and handle_char
    status = multi-target-composed or multi-target-partial-composed
  run::x should receive guarded imported pointer-use evidence:
    int * deref-readwrite under tag
    char * deref-readwrite under !(tag)

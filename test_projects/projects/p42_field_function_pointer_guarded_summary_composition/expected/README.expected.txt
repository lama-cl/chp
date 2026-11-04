Expected C5f behavior:

1. C5a/C5d reporting should still show guarded field-function-pointer alternatives:
     run::ops.process -> handle_int   [guard=tag]
     run::ops.process -> handle_char  [guard=!(tag)]

2. C5c same-TU propagation should still resolve ops->process(x) to both handlers.

3. New in C5f: the handler summaries propagated to run::$1 should preserve
   the field-target selection guards in the cast-history summary, not just in
   the call-resolution report.  The exact formatter may vary, but the function
   summary / direct cast history for run::x should contain guarded alternatives
   rather than only an unguarded parallel:

     expected shape:
       run($0 : struct Ops *,
           $1 : ... [guard=tag] ... int * ... [guard=!(tag)] ... char * ...,
           $2 : int)

     no longer sufficient by itself:
       run($1 : void * => <| char *, int * |>)

4. Pointer-use summaries for run::x should retain the existing guarded operation
   evidence from C5c where available:
     [tag]     int * deref-readwrite
     [!(tag)]  char * deref-readwrite

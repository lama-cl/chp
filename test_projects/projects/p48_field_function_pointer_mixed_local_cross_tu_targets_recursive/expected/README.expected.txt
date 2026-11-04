Expected C6c1 behavior:
  This project extends p43 with a cross-TU recursive SCC:
    local_handle    is defined in a.c and calls external_handle(p)
    external_handle is defined in b.c and calls local_handle(p)

  run dispatches through a guarded field-function-pointer target set:
    [tag]     ops->process -> local_handle
    [!(tag)]  ops->process -> external_handle

  The project-solved summary for run.$1 should preserve the local target wrapper
  but should not keep the raw first projection from the same field-FP edge as a
  parallel sibling.  The desired shape is conceptually:

    run($0 : struct Ops *,
        $1 : void * => ⟨[tag] local_handle⌜int * => void * => char * => void *⌝local_handle,
                        [!(tag)] external_handle⌜char * => void * => int * => void *⌝external_handle⟩,
        $2 : int)

  The following shape is the regression this project guards against:

    [tag] ⟨| local_handle⌜int * => void * => char * => void *⌝local_handle,
             int * |⟩

  The suppression must be same-edge specific: same call site, guard, field
  entity, target function, caller actual parameter, and callee formal parameter.
  It must not be implemented as a generic rule that removes every raw int *
  whenever a wrapper containing int * is nearby.


Expected C6c2 behavior:
  The guarded alternatives should be displayed in the source order of the
  field-function-pointer target assignments, not in target-name/import order:

    [tag]     local_handle      // assignment in the then branch
    [!(tag)]  external_handle   // assignment in the else branch

  The name external_handle must not by itself trigger ExternalUnknown or
  ExternalSink when the callee was resolved/imported and its pointer operations
  were materialized by summary import / project-SCC propagation.

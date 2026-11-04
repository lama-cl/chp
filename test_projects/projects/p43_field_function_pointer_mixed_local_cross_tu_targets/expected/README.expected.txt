Expected C6c behavior:
  This project has a mixed field-function-pointer target set:
    [tag]     local_handle      is defined in a.c
    [!(tag)]  external_handle   is imported from b.c

  The call resolution for run via ops->process should preserve both branches:
    run via ops->process -> local_handle     [same-tu-field-function-pointer-call] [guard=tag]
    run via ops->process -> external_handle  [cross-tu-guarded-field-function-pointer-target-set] [guard=!(tag)]

  C6c should make both pointer-operation branches visible on run::x:
    [tag]     origin=field-fnptr, target=local_handle,    field=run::ops.process, deref-readwrite as=int *,  access=int
    [!(tag)]  origin=field-fnptr, target=external_handle, field=run::ops.process, deref-readwrite as=char *, access=char

  The local branch should no longer appear only inside function-parameter
  behavior metrics; it should also be visible in entity-level Pointer use
  summaries and Grouped pointer-operation summaries.  The external branch should
  continue to come from summary import.

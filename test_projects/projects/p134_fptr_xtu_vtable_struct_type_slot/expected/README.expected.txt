Expected fptr-extension behavior for p134_fptr_xtu_vtable_struct_type_slot.

Fact: X03 xtu-vtable-struct-type-slot

Fact X03 (cross-TU F15): the ops instance and constructor live in b.c; the
virtual call o->vt->read(o->data) is in a.c.  Requires the struct-type slot
`struct ops::read` to be a project-wide L1 fact.
Real world: libarchive archive_vtable installed per format and called from
archive_virtual.c:36.

Subject obj_read::o.data:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=int* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p134_fptr_xtu_vtable_struct_type_slot
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

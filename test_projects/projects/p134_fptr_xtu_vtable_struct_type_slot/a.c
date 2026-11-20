/*
 * Fact X03 (cross-TU F15): the ops instance and constructor live in b.c; the
 * virtual call o->vt->read(o->data) is in a.c.  Requires the struct-type slot
 * `struct ops::read` to be a project-wide L1 fact.
 *
 * Real world: libarchive archive_vtable installed per format and called from
 * archive_virtual.c:36.
 *
 * @fptr-fact X03 xtu-vtable-struct-type-slot
 * @subject   obj_read::o.data
 * @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
 */
#include "ops.h"

int obj_read(struct obj *o) { return o->vt->read(o->data); }

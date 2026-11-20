Expected fptr-extension behavior for p139_fptr_xtu_oop_void_self_downcast.

Fact: X08 xtu-oop-void-self-downcast

Fact X08 (cross-TU F16): the generic dispatcher shape_area() is in a.c; the
derived class (circle), its method and vtable are in b.c.  The type of
`self` at the dispatch site comes from b.c's method summary.
Real world: icecast format plugins (format.h:57, format_ogg.c:279,
format_mp3.c:87).

Subject shape_area::self:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
  required after extension: category=MonomorphicActiveSpecialization types=struct_circle* resolution=exact@L1

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p139_fptr_xtu_oop_void_self_downcast
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

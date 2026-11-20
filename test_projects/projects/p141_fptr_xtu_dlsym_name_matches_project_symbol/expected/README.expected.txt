Expected fptr-extension behavior for p141_fptr_xtu_dlsym_name_matches_project_symbol.

Fact: X10 xtu-dlsym-name-matches-project-symbol

Fact X10 (cross-TU F23): dlsym(lib, "plugin_entry") where a function named
plugin_entry also exists in the project (b.c).  Decision D6: the result stays
ExternalUnknown; the name match may only be reported as a low-confidence
candidate hint and must NOT be composed into the history (the loaded object
need not be this project's build).
Real world: Lua loadlib.c:141; ImageMagick module.c:1302.

Subject run::ctx:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
  required after extension: category=ExternalUnknown resolution=external-unknown@LIB

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p141_fptr_xtu_dlsym_name_matches_project_symbol
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md

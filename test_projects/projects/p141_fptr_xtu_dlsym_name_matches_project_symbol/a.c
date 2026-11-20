/*
 * Fact X10 (cross-TU F23): dlsym(lib, "plugin_entry") where a function named
 * plugin_entry also exists in the project (b.c).  Decision D6: the result stays
 * ExternalUnknown; the name match may only be reported as a low-confidence
 * candidate hint and must NOT be composed into the history (the loaded object
 * need not be this project's build).
 *
 * Real world: Lua loadlib.c:141; ImageMagick module.c:1302.
 *
 * @fptr-fact X10 xtu-dlsym-name-matches-project-symbol
 * @subject   run::ctx
 * @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
 * @expected  category=ExternalUnknown resolution=external-unknown@LIB
 */
#include <dlfcn.h>

typedef int (*handler_t)(void *);

int run(void *lib, void *ctx) {
  handler_t cb = (handler_t)dlsym(lib, "plugin_entry");
  return cb(ctx);
}

// fptr-23-dlsym-plugin.c
//
// Fact F23: a function pointer obtained from dlsym().  The target lives in a
// shared object that is not part of the analyzed project, so no target set can
// be computed.  Decision D6: classify the context as ExternalUnknown with
// resolution external-unknown (not "unresolved"), so analysis imprecision and
// a true dynamic-linking boundary are reported differently.
//
// Real world: Lua's C module loader
//   lua_CFunction f = cast_func(dlsym(lib, sym));
// (test_projects/bench-3c/lua-5.4.1/src/loadlib.c:141); ImageMagick coder
// modules via lt_dlsym (MagickCore/module.c:1302).
//
// @fptr-fact F23 dlsym-plugin
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
// @expected  category=ExternalUnknown resolution=external-unknown@LIB
#include <dlfcn.h>

typedef int (*handler_t)(void *);

int run(void *lib, void *ctx) {
  handler_t cb = (handler_t)dlsym(lib, "plugin_entry");
  return cb(ctx);
}

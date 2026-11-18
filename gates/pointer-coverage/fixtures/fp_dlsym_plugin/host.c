// GATE fp-dlsym-plugin (TU 1 of 2: host)
// Shared-object dynamic loading.  The entry point is looked up by string
// name; the plugin definition lives in another TU of the same project.
//
// Probe subjects:
//   host_run.$0 (data) -- dlsym("plugin_entry") must link to the project
//                         definition and import its struct job * downcast.
//   host_run_unknown.$0 -- symbol name is not a literal: must be reported
//                         as an explicit dynamic-symbol boundary.
#include <dlfcn.h>

typedef int (*entry_fn)(void *data);

int host_run(void *data, const char *path) {
  void *h = dlopen(path, RTLD_NOW);
  if (!h)
    return -1;
  entry_fn fn = (entry_fn)dlsym(h, "plugin_entry");
  int rc = fn ? fn(data) : -1;
  dlclose(h);
  return rc;
}

int host_run_unknown(void *data, void *h, const char *sym) {
  entry_fn fn = (entry_fn)dlsym(h, sym);
  return fn ? fn(data) : -1;
}

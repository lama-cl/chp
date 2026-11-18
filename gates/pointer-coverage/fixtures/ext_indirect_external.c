// GATE P0-escape-accounting (indirect call to a library function)
// An allocator hook whose only known target is libc `free`.  The call is
// indirect, but the target is known: the loss is an external call, not an
// unresolved callback.  Found on parson (parson_free = free).
//
// Probe subjects:
//   release.$0 (p)  -- escape.reason external-unmodeled, externalCallees [free],
//                      site kind indirect-external
#include <stdlib.h>

static void (*hook_free)(void *) = free;

void release(void *p) {
  hook_free(p);
}

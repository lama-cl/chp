// fptr-07-named-command-table.c
//
// Fact F07: an array of {name, function} records searched by a loop, and the
// selected record's function is called through a record pointer.  Requires
// struct-array element storage facts (cmds[*].fn) and a loop-carried pointer
// into the array.  Today callbackMediation already says
// GuardedResolvedFieldCallback, yet no int* evidence reaches run::ctx: the
// mediation label is set from provenance, not from a successful transfer.
//
// Real world: luaL_setfuncs walks `const luaL_Reg *l` with
//   for (; l->name != NULL; l++) ... l->func
// (test_projects/bench-3c/lua-5.4.1/src/lauxlib.c:904-913).
//
// @fptr-fact F07 named-command-table
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=GuardedResolvedFieldCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
#include <string.h>

typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_int_neg(void *p) { int *q = p; return -*q; }

struct command { const char *name; handler_t fn; };

static const struct command cmds[] = {
  { "get", use_int },
  { "neg", use_int_neg },
  { 0, 0 },
};

int run(const char *name, void *ctx) {
  for (const struct command *c = cmds; c->name; c++)
    if (strcmp(c->name, name) == 0)
      return c->fn(ctx);
  return -1;
}

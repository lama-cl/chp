// GATE fp-callback-registered
// A callback and its context are registered into file-scope storage and
// invoked later from a different function.  The context's concrete
// interpretation (int *) only exists inside the handler.
//
// Probe subjects:
//   register_handler.$1 (ctx)  -- must inherit handler's int* interpretation
//   fire: indirect call g_cb(g_ctx) must resolve to {on_event}
typedef void (*event_cb)(void *ctx);

static event_cb g_cb;
static void *g_ctx;

static void on_event(void *ctx) {
  int *counter = (int *)ctx;
  *counter += 1;
}

void register_handler(event_cb cb, void *ctx) {
  g_cb = cb;
  g_ctx = ctx;
}

void fire(void) {
  if (g_cb)
    g_cb(g_ctx);
}

int demo(void) {
  int counter = 0;
  register_handler(on_event, &counter);
  fire();
  return counter;
}

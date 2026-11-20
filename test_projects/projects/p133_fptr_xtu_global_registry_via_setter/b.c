/*
 * @fptr-fact X02 xtu-global-registry-via-setter
 * @subject   fire::ctx
 * @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
 */
typedef int (*handler_t)(void *);

static handler_t g_handler;

void set_handler(handler_t fn) { g_handler = fn; }

int fire(void *ctx) { return g_handler(ctx); }

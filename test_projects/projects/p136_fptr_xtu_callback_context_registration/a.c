/*
 * Fact X05 (cross-TU F18): the registry (add/fire) is a library TU (b.c); the
 * client in a.c registers (use_int, x).  b.c's summary of add() must export a
 * callback-context binding "$0 is invoked later with $1 as its argument #0",
 * which a.c applies at the registration site.
 *
 * Real world: c-thread-pool thpool_add_work (main.c:345) / worker call
 * (main.c:530).
 *
 * @fptr-fact X05 xtu-callback-context-registration
 * @subject   setup::x
 * @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
 */
typedef int (*handler_t)(void *);

void add(handler_t fn, void *arg);

static int use_int(void *p) { int *q = p; return *q; }

void setup(void *x) { add(use_int, x); }

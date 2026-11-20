/*
 * Fact X04 (cross-TU F11): pick() in b.c returns a *static* function of b.c.
 * a.c cannot name use_int, so the returned target must be identified by a
 * TU-qualified lookup key (static functions are not public symbols).
 *
 * @fptr-fact X04 xtu-factory-returns-static-target
 * @subject   run::ctx
 * @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
 */
typedef int (*handler_t)(void *);

handler_t pick(void);

int run(void *ctx) {
  handler_t cb = pick();
  return cb(ctx);
}

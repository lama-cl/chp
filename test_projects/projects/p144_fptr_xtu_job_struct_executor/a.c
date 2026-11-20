/*
 * Fact X13 (cross-TU F20): the executor exec(struct job *) is in b.c and calls
 * j->fn(j->arg); a.c builds the job with its own callback.  b.c's summary is
 * relative to $0->fn and $0->arg and must be specialized at a.c's call.
 *
 * Real world: bzip2 bz_stream bzalloc/opaque (main.c:112, 7857).
 *
 * @fptr-fact X13 xtu-job-struct-executor
 * @subject   submit::j.arg
 * @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
 */
typedef int (*handler_t)(void *);
struct job { handler_t fn; void *arg; };

int exec(struct job *j);

static int use_int(void *p) { int *q = p; return *q; }

int submit(void *ctx) {
  struct job j = { use_int, ctx };
  return exec(&j);
}

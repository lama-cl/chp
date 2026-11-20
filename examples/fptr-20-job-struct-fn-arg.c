// fptr-20-job-struct-fn-arg.c
//
// Fact F20: callback and context travel together in a struct passed by
// pointer to an executor that calls j->fn(j->arg).  The executor's summary is
// relative to its parameter ($0->fn applied to $0->arg); the caller supplies a
// struct whose fn field is known.
//
// Real world: bzip2's bz_stream carries bzalloc and opaque together and calls
// (*(strm->bzalloc))(strm->opaque, ...)
// (test_projects/bench-genc2rust/bzip2/main.c:112, 7857).
//
// @fptr-fact F20 job-struct-fn-arg
// @subject   submit::j.arg
// @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

struct job { handler_t fn; void *arg; };

static int exec(struct job *j) { return j->fn(j->arg); }

int submit(void *ctx) {
  struct job j = { use_int, ctx };
  return exec(&j);
}

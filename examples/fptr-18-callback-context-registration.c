// fptr-18-callback-context-registration.c
//
// Fact F18: callback + void * context registration.  add() stores the pair in
// a global array; fire() later calls regs[i].fn(regs[i].arg).  The data
// pointer's type is decided by the callback paired with it, so the useful
// place to classify setup::x is the *registration* site where the pair is
// visible (callback-context binding).
//
// Real world: c-thread-pool thpool_add_work(pool, function_p, arg_p)
// (test_projects/bench-genc2rust/c-thread-pool/main.c:345) queues the pair;
// the worker later calls job_p->function(job_p->arg) (main.c:530).
// Also thttpd tmr_create(..., TimerProc*, ClientData, ...) (timers.h:72,
// fired at timers.c:266).
//
// @fptr-fact F18 callback-context-registration
// @subject   setup::x
// @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

struct reg { handler_t fn; void *arg; };
static struct reg regs[8];
static int nregs;

void add(handler_t fn, void *arg) {
  regs[nregs].fn = fn;
  regs[nregs].arg = arg;
  nregs++;
}

void fire(void) {
  for (int i = 0; i < nregs; i++)
    regs[i].fn(regs[i].arg);
}

void setup(void *x) { add(use_int, x); }

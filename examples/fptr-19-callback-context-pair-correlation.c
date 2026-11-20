// fptr-19-callback-context-pair-correlation.c
//
// Fact F19 (precision): two registrations pair different callbacks with
// different contexts.  A flow-insensitive storage model of regs[*].fn and
// regs[*].arg separately would cross the pairs and make BOTH contexts look
// int*/double* (spurious multi-target).  The binding must be kept per
// registration site: setup_int::x is int* only.
//
// Real world: any callback registry used by more than one client, e.g. the
// thttpd timer list (timers.c:266) holding timers of different ClientData
// kinds.
//
// @fptr-fact F19 callback-context-pair-correlation
// @subject   setup_int::x
// @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
// @subject   setup_dbl::y
// @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L1
typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

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

void setup_int(void *x) { add(use_int, x); }
void setup_dbl(void *y) { add(use_dbl, y); }

/*
 * Fact X06 (cross-TU F19, precision): two clients register different
 * (callback, context) pairs with the b.c registry.  Each context must be
 * classified by its own callback only; crossing the pairs would make both look
 * both int* and double*.
 *
 * @fptr-fact X06 xtu-callback-pair-correlation
 * @subject   setup_int::x
 * @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
 * @subject   setup_dbl::y
 * @baseline  category=OpaqueTransportOrViewOnly callback=None behavior=OpaqueForwarding types=-
 * @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L1
 */
typedef int (*handler_t)(void *);

void add(handler_t fn, void *arg);

static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }

void setup_int(void *x) { add(use_int, x); }
void setup_dbl(void *y) { add(use_dbl, y); }

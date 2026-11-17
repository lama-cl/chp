// GATE fp-array-dispatch
// Indirect call through a constant function-pointer array.  Each arm
// interprets the same void* payload as a different concrete type.
//
// Probe subjects:
//   dispatch.$1 (payload) -- must become a parallel history {int *, double *}
//   dispatch: call handlers[op](payload) must resolve to {h_int, h_dbl}
//   precision: h_unrelated has the same signature and is address-taken, but
//   never enters `handlers`; its long * view must NOT reach dispatch.$1
typedef void (*handler_fn)(void *payload);

static void h_int(void *payload) {
  int *pi = (int *)payload;
  *pi = 1;
}

static void h_dbl(void *payload) {
  double *pd = (double *)payload;
  *pd = 1.0;
}

static void h_unrelated(void *payload) {
  long *pl = (long *)payload;
  *pl = 1;
}

static const handler_fn handlers[] = {h_int, h_dbl};

void dispatch(unsigned op, void *payload) {
  handlers[op](payload);
}

void demo(void) {
  int i = 0;
  double d = 0;
  dispatch(0, &i);
  dispatch(1, &d);
}

void unrelated_demo(void) {
  long l = 0;
  handler_fn other = h_unrelated;
  other(&l);
}

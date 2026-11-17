// GATE ext-unknown-boundary
// A pointer escapes to a function with no body and no model.  Nothing can be
// inferred -- but the loss must be *accounted for*: the subject must carry an
// explicit escape reason naming the callee, never silently look "unused" or
// "transparent".
//
// Probe subjects:
//   leak.$0 (p)  -- escape reason must name vendor_consume
//   keep.$0 (p)  -- control: no escape, plain int * interpretation
void vendor_consume(void *opaque);

void leak(void *p) {
  vendor_consume(p);
}

int keep(void *p) {
  int *pi = (int *)p;
  return *pi;
}

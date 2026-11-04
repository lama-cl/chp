// C6g4m regression: interprocedural pointer-arithmetic-run subsumption.
//
// The local source-level history retains both distinct p += statements.  When
// the value crosses sink(), however, repeated same-type ptrarith(char *)
// components form one idempotent pointer-use attachment run rather than
// multiplying the single sink-side dereference once per run length.

static void sink(char *p) {
  *p = 0;
}

void distinct_local_steps_then_call(char *p) {
  p += 1;
  p += 2;
  sink(p);
}

// A cast boundary must remain visible: the transfer normalizer may not merge
// ptrarith(int *) and ptrarith(char *) across the explicit cast.
static void sink_bytes(char *p) {
  *p = 1;
}

void cast_boundary_is_not_collapsed(int *p) {
  p += 1;
  sink_bytes((char *)(p + 2));
}

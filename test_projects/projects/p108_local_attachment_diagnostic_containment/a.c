/*
 * p108_local_attachment_diagnostic_containment/a.c
 *
 * C6g4x1a regression fixture.  Several pointer-arithmetic continuations feed
 * a single local call use.  The diagnostic stream may truncate or disable
 * itself, but it must never change the cast-history result or abort traversal.
 */

typedef void (*sink_fn)(char *);

static void sink(char *p) {
  (void)p;
}

static char *walk(char *p, int n, sink_fn cb) {
  char *q = p;
  for (int i = 0; i < n; ++i)
    q = q + 1;
  cb(q + 1);
  return q;
}

int main(void) {
  char bytes[32] = {0};
  (void)walk(bytes, 4, sink);
  return 0;
}

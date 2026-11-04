/*
 * p121_named_call_scc_local_worklist/a.c
 *
 * C6g5b regression: a two-function recursive SCC must be drained locally
 * before the entry caller is activated.  The unrelated branch must not become
 * part of the recursive component or be re-executed because scc_a/scc_b
 * exchange a refined summary.
 */

static void leaf_cast_and_use(void *p);
static void scc_a(void *p);
static void scc_b(void *p);
static void unrelated_int(void *p);

void exercise_named_call_scc_local_worklist(void *primary, void *unrelated) {
  scc_a(primary);
  unrelated_int(unrelated);
}

static void scc_a(void *p) {
  scc_b(p);
  leaf_cast_and_use(p);
}

static void scc_b(void *p) {
  scc_a(p);
}

static void leaf_cast_and_use(void *p) {
  unsigned char *bytes = (unsigned char *)p;
  bytes[0] ^= 1;
}

static void unrelated_int(void *p) {
  int *slots = (int *)p;
  slots[0] += 1;
}

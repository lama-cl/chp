/*
 * p120_named_call_dependency_worklist/a.c
 *
 * C6g5b regression: a delayed, four-stage named-call chain should re-enqueue
 * only the three upstream consumers after the leaf summary becomes available.
 * The two unrelated entry branches are seeded once but must not be revisited
 * while the primary chain is refined.
 */

static void leaf_cast_and_use(void *p);
static void relay_one(void *p);
static void relay_two(void *p);
static void relay_three(void *p);
static void unrelated_int(void *p);
static void unrelated_long(void *p);

void exercise_named_call_dependency_worklist(void *primary,
                                             void *first_unrelated,
                                             void *second_unrelated) {
  relay_three(primary);
  unrelated_int(first_unrelated);
  unrelated_long(second_unrelated);
}

static void relay_three(void *p) {
  relay_two(p);
}

static void relay_two(void *p) {
  relay_one(p);
}

static void relay_one(void *p) {
  leaf_cast_and_use(p);
}

static void leaf_cast_and_use(void *p) {
  unsigned char *bytes = (unsigned char *)p;
  bytes[0] ^= 1;
}

static void unrelated_int(void *p) {
  int *slots = (int *)p;
  slots[0] += 1;
}

static void unrelated_long(void *p) {
  long *slots = (long *)p;
  slots[0] += 1;
}

/*
 * C6g5f paper-facts fixture.
 *
 * The pointer subject `p` has all three directional array-index operations,
 * direct pointer arithmetic, and a resolved callback target with a concrete
 * dereference.  The paper-facts sidecar should expose one subject-row with
 * arrayIndexRead/Write/ReadWrite, pointerArithmetic, and callback-use facts.
 */

typedef int (*callback_t)(void *);

static int consume_int(void *raw) {
  int *value = (int *)raw;
  return *value;
}

struct callback_box {
  callback_t callback;
};

int paper_facts_array_operations(unsigned char *bytes,
                                 struct callback_box *box) {
  int *p = (int *)bytes;
  int first = p[0];
  p[1] = first;
  p[2] += 1;
  p = p + 1;
  return box->callback(p);
}

/*
 * C6g4z4 regression: one immutable direct-call callback-binding snapshot must
 * be consumed consistently by both AST-time call reporting and named-call
 * summary propagation.
 *
 * `compare_identity` has no parameter cast history.  Therefore:
 *   invoke.$0 : const void * => $2.$0
 *   invoke.$1 : const void * => $2.$1
 * but top.$0 has no cast-history summary at all after specializing the unique
 * callback `compare_identity`: both callback references are identities.
 */

typedef int (*compare_fn)(const void *, const void *);

static int invoke(const void *left, const void *right, compare_fn compare) {
  return compare(left, right);
}

static int compare_identity(const void *left, const void *right) {
  return left != right;
}

int top(const void *value) {
  return invoke(value, value, compare_identity);
}

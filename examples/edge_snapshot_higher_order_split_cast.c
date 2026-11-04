/*
 * C6g4z3 regression: a uniquely bound, cast-free callback must specialize
 * generic $fn.$arg references from the NamedCallEdge snapshot.  The caller
 * summary must retain the non-identity char-pointer path but must not leak
 * bridge.$0 / bridge.$1 symbolic callback references.
 */

typedef int (*cmp_fn)(const void *, const void *);

static int bridge(cmp_fn compare, const void *value) {
  const char *bytes = (const char *)value;
  return compare(value, bytes);
}

static int compare_identity(const void *left, const void *right) {
  int* pi = left;
  double* pd = right;
  return left != right;
}

int call_bridge(const void *value) {
  return bridge(compare_identity, value);
}

/*
 * C6g4z2 regression: when a direct call binds a function-pointer parameter to
 * one concrete, locally defined callback whose selected parameter has no cast
 * observations, the callback formal reference is an identity and must not
 * escape into the caller summary as $0.$N or <unknown:...-unsummarized>.
 */

typedef int (*predicate_fn)(const void *, const void *);

static int invoke_predicate(predicate_fn predicate,
                            const void *left,
                            const void *right) {
  return predicate(left, right);
}

static int identity_predicate(const void *left, const void *right) {
  return left != right;
}

int run_identity_predicate(const void *value) {
  return invoke_predicate(identity_predicate, value, value);
}

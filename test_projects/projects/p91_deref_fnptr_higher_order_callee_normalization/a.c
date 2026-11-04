/*
 * p91_deref_fnptr_higher_order_callee_normalization/a.c
 *
 * C6g3 regression: a dereferenced function-pointer parameter in call position
 * must be normalized to the same higher-order callee identity as the ordinary
 * function-pointer call spelling.
 */

typedef unsigned long size_t;
typedef int (*compare_fn)(const void *, const void *);

static int compare_ints(const void *lhs, const void *rhs) {
  const int *left = lhs;
  const int *right = rhs;
  return (*left > *right) - (*left < *right);
}

static const void *at_index(const void *base, size_t index, size_t size) {
  return (const void *)((const char *)base + index * size);
}

void invoke_direct(compare_fn compare,
                   const void *key,
                   const void *base,
                   size_t index,
                   size_t size) {
  const void *candidate = at_index(base, index, size);
  (void)compare(key, candidate);
}

void invoke_deref(compare_fn compare,
                  const void *key,
                  const void *base,
                  size_t index,
                  size_t size) {
  const void *candidate = at_index(base, index, size);
  (void)(*compare)(key, candidate);
}

void exercise(const int *key, const int *base) {
  invoke_direct(compare_ints, key, base, 0UL, sizeof(*base));
  invoke_deref(compare_ints, key, base, 0UL, sizeof(*base));
}

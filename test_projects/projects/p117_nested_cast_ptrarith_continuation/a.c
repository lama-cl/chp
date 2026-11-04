/*
 * C6g4z1 regression: nested meaningful casts around pointer arithmetic must
 * remain one sequential reaching-origin path.  The nested void* spelling is
 * emitted by minimap2-style transformed sources.
 *
 * Required shape for both $1 summaries:
 *   const void * => const char * => ptrarith(const char *)
 *   => [void * =>] const void * => $3.$1
 *
 * In particular, bsearch_nested_void.$1 must not report Parallel arms for the
 * assignment projection and the later comparator argument.
 */

typedef unsigned long size_t;
typedef int (*cmp_fn)(const void *, const void *);

static int bsearch_simple(const void *key, const void *base,
                          size_t width, cmp_fn compare) {
  const void *p;
  p = (const void *)((const char *)base + width);
  return compare(key, p);
}

static int bsearch_nested_void(const void *key, const void *base,
                               size_t width, cmp_fn compare) {
  const void *p;
  p = (const void *)((void *)((const char *)base + width));
  return compare(key, p);
}

static int compare_bytes(const void *left, const void *right) {
  return left != right;
}

int run_nested_cast_ptrarith(const void *base, size_t width) {
  return bsearch_simple(base, base, width, compare_bytes) +
         bsearch_nested_void(base, base, width, compare_bytes);
}

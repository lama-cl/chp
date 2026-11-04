/*
 * p127_guarded_recursive_ptrarith_membership_projection/a.c
 *
 * C6g5q regression: each callee creates the same guarded byte-pointer
 * arithmetic transformer. Recursive replay must not treat re-nesting that
 * transformer as a new cast-history SCC state.
 */

typedef unsigned char u8;

static void g(u8 *p, int tag);

static void f(u8 *p, int tag) {
  u8 *q;
  if (tag)
    q = p + 1;
  else
    q = p + 2;
  g(q, tag);
}

static void g(u8 *p, int tag) {
  u8 *q;
  if (tag)
    q = p + 3;
  else
    q = p + 4;
  f(q, tag);
}

int run_guarded_recursive_ptrarith_membership_projection(u8 *input, int tag) {
  f(input, tag);
  return 0;
}

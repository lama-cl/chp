/*
 * p101_source_leaf_identity/a.c
 *
 * C6g4s/C6g4t regression: preserve an immutable AST pointer-use operation
 * site and operation-local guard through alias relays and recursive-source
 * named-call exports.  C6g4t must use that immutable site, rather than each
 * mutable transfer span, for named-call solver membership.
 */

static void recursive_head(void *p, int depth);

static void shared_leaf(void *p) {
  int *ip = (int *)p;
  *ip += 1;                 /* one leaf, reached through distinct transfer sites */
}

static void recursive_body(void *p, int depth) {
  if (depth > 0) {
    int *ip = (int *)p;
    *ip += 1;               /* leaf A: guarded by depth > 0 */
    shared_leaf(p);
    recursive_head(p, depth - 1);
  } else {
    char *cp = (char *)p;
    *cp += 1;               /* leaf B: guarded by !(depth > 0) */
    void *alias = p;
    shared_leaf(alias);     /* same shared_leaf source site via alias */
  }
}

static void recursive_head(void *p, int depth) {
  recursive_body(p, depth);
}

void export_recursive_summary(void *p) {
  /* acyclic caller -> recursive callee: recursive-source export path */
  recursive_head(p, 1);
}

void exercise_source_leaf_identity(int *value) {
  export_recursive_summary(value);
}

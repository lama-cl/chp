/*
 * p107_local_pointer_use_leaf_projected_diagnostics/a.c
 *
 * C6g4x1 regression fixture.  Both branch alternatives feed the same call
 * leaf.  Their reaching guards differ, and one alternative carries a longer
 * same-type pointer-arithmetic run.  The diagnostic must compare these as
 * bounded variants inside one leaf-projected group; it must not use the
 * effective guard, mutable span, or provenance to split the group.
 */

static void consume(char *p) {
  if (p)
    *p = '\0';
}

void local_pointer_use_leaf_projected_diagnostics(char *input, int choose) {
  char *cursor;
  if (choose)
    cursor = (input + 1) + 2;
  else
    cursor = input + 3;
  consume(cursor);
}

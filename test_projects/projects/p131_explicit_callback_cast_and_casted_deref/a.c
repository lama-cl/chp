/*
 * C6g5f3 regression:
 *
 * 1. A direct callback actual wrapped in an explicit compatible function-pointer
 *    cast must resolve to the same concrete FunctionDecl as the bare designator.
 *    The generic dispatcher summary retains $2.$0/$2.$1, but concrete caller
 *    histories must substitute these references away.
 *
 * 2. A direct load through *(char **)opaque records `void * => char **` on
 *    the source parameter.  The loaded char * is not a second cast of opaque.
 */

typedef int (*compare_fn)(void *, void *);

static int cmpstr(void *v1, void *v2) {
  char *s1 = *(char **)v1;
  char *s2 = *(char **)v2;
  return *s1 != *s2;
}

static int cmpnum(void *v1, void *v2) {
  int n1 = *(int *)v1;
  int n2 = *(int *)v2;
  return n1 - n2;
}

static void dispatch(void *left, void *right, compare_fn compare) {
  (void)compare(left, right);
}

void exercise_plain(int *values) {
  dispatch(values, values, cmpnum);
}

void exercise_casted_int(int *values) {
  dispatch(values, values, (compare_fn)cmpnum);
}

void exercise_casted_strings(char **values) {
  dispatch(values, values, (compare_fn)cmpstr);
}

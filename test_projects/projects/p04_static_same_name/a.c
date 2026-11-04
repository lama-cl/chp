/*
 * p04_static_same_name/a.c
 *
 * This file has a static helper named helper.
 */

static void helper(void *x) {
  int *p = x;
  *p += 1;
}

void fa(void *x) {
  helper(x);
}

int main(void) {
  int i = 0;
  fa(&i);
  return i;
}

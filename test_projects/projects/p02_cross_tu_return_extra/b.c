/*
 * p02_cross_tu_return/b.c
 *
 * g specializes y to char* and returns that value as void*.
 */

void *g(void *y) {
  char *p = y;
  return p;
}

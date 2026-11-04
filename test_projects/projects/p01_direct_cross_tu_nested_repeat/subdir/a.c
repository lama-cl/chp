/*
 * p01_direct_cross_tu/b.c
 *
 * g specializes its void* parameter to int* and writes through it.
 */

void g(void *y) {
  char* pc = y;
  void* pv = pc;
  int *p = y;
  *p += 1;
}

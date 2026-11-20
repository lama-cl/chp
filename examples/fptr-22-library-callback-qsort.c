// fptr-22-library-callback-qsort.c
//
// Fact F22: libc qsort() calls the comparator with pointers to *elements* of
// base (base + i*size), not with base itself.  The model must transfer the
// comparator's parameter view onto base through an element-address relation.
//
// Required library model:
//   qsort: fn=arg#3, data=arg#0 -> fn params #0,#1 as element addresses.
//
// Real world: thttpd sorts its date tables with qsort
// (test_projects/bench-3c/thttpd-2.29/tdate_parse.c:114).
//
// @fptr-fact F22 library-callback-qsort
// @subject   sort_ints::base
// @baseline  category=ExternalUnknown callback=None behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=const_int* resolution=exact@LIB
#include <stdlib.h>

static int cmp_int(const void *a, const void *b) {
  const int *x = a;
  const int *y = b;
  return *x - *y;
}

void sort_ints(void *base, size_t n) { qsort(base, n, sizeof(int), cmp_int); }

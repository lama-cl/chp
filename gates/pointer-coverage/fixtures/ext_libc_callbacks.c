// GATE ext-libc-callbacks
// Pointers escape into well-known library functions that call back into
// project code.  The library body is unavailable, but its contract is known.
//
// Probe subjects:
//   sort_ints.$0 (base : void *)  -- qsort feeds elements of base to
//                                    cmp_int.$0/$1, which read them as int *
//   spawn.$0 (arg : void *)       -- pthread_create forwards arg to
//                                    worker.$0, which downcasts to struct job *
#include <pthread.h>
#include <stdlib.h>

struct job {
  int id;
};

static int cmp_int(const void *a, const void *b) {
  const int *x = (const int *)a;
  const int *y = (const int *)b;
  return (*x > *y) - (*x < *y);
}

void sort_ints(void *base, size_t n) {
  qsort(base, n, sizeof(int), cmp_int);
}

static void *worker(void *arg) {
  struct job *j = (struct job *)arg;
  j->id++;
  return 0;
}

int spawn(void *arg) {
  pthread_t t;
  return pthread_create(&t, 0, worker, arg);
}

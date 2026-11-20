// fptr-21-library-callback-pthread-create.c
//
// Fact F21: a library function (not project code) invokes the callback and
// forwards the context: pthread_create(&t, attr, worker, arg) calls
// worker(arg).  Today spawn::ctx is ExternalUnknown although worker's body
// specializes it to int*.
//
// Required: a curated library callback model
//   pthread_create: fn=arg#2, data=arg#3 -> fn param #0.
//
// Real world: c-thread-pool starts workers with pthread_create
// (test_projects/bench-genc2rust/c-thread-pool/main.c:470).
//
// @fptr-fact F21 library-callback-pthread-create
// @subject   spawn::ctx
// @baseline  category=ExternalUnknown callback=None behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@LIB
#include <pthread.h>

static void *worker(void *arg) {
  int *q = arg;
  *q = 1;
  return 0;
}

int spawn(void *ctx) {
  pthread_t t;
  return pthread_create(&t, 0, worker, ctx);
}

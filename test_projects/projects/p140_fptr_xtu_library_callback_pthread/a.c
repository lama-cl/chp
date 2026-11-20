/*
 * Fact X09 (cross-TU F21): the thread entry point is defined in b.c and passed
 * to pthread_create in a.c.  The library model provides the binding
 * (fn=#2, data=#3 -> param #0); the target summary is imported from b.c.
 *
 * @fptr-fact X09 xtu-library-callback-pthread
 * @subject   spawn::ctx
 * @baseline  category=ExternalUnknown callback=None behavior=OpaqueForwarding types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@LIB
 */
#include <pthread.h>

void *worker(void *arg);

int spawn(void *ctx) {
  pthread_t t;
  return pthread_create(&t, 0, worker, ctx);
}

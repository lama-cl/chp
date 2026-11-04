/*
 * p05_function_pointer_target/a.c
 *
 * run passes a cross-TU function as a callback target.
 */

void *idv(void *);
void ho(void *(*pf)(void *), void *x);

void run(void *x) {
  ho(idv, x);
}

int main(void) {
  int i = 0;
  run(&i);
  return i;
}

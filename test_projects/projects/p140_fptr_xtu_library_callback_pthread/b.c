void *worker(void *arg) {
  int *q = arg;
  *q = 1;
  return 0;
}
